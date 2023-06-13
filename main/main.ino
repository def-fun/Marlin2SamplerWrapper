/*
Arduino Modbus 从站
todo 自动计算所有样品瓶的坐标
todo 检查marlin返回的数据之后，再响应modbus
https://www.heibing.org/2019/12/136
*/
#include <Arduino.h>
#include <Regexp.h>

/*通讯参数*/
#define bufferSize 255      //modbus一帧数据的最大字节数量
#define baudrate 9600       //定义所有通讯的波特率
#define slaveID 20          //定义modbus RTU从站站号，20 == 0x14
#define modbusDataSize 100  //定义modbus数据库空间大小
#define LED_BUILTIN 2       //板载LED
#define RX1 18              //Serial1，转RS485 modbus和上位机通信
#define TX1 19
#define RX2 16  //Serial2，TTL，和marlin通信
#define TX2 17

/*提供样品盘参数，基于正方形网格计算每个瓶子的坐标. */
#define SAMPLE_X_NUM 5        //x方向有5列
#define SAMPLE_Y_NUM 5        //y方向有5列
#define SAMPLE_POS_1_X 10     //第一个瓶子的x坐标
#define SAMPLE_POS_1_Y 10     //第一个瓶子的y坐标
#define SAMPLE_POS_END_X 110  //和第一个瓶子成对角线的瓶子的x坐标
#define SAMPLE_POS_END_Y 110  //和第一个瓶子成对角线的瓶子的y坐标
#define COLLECTOR_POS_X 0     //废液瓶的x坐标
#define COLLECTOR_POS_Y 10    //废液瓶的y坐标
float POINTS_X[26] = { 0.0, 0.0, -0.0, -0.0, -0.0, -0.0, 25.0, 25.0, 25.0, 25.0, 25.0, 50.0, 50.0, 50.0, 50.0, 50.0, 75.0, 75.0, 75.0, 75.0, 75.0, 100.0, 100.0, 100.0, 100.0, 100.0 };
float POINTS_Y[26] = { 0.0, 0.0, 25.0, 50.0, 75.0, 100.0, 0.0, 25.0, 50.0, 75.0, 100.0, 0.0, 25.0, 50.0, 75.0, 100.0, 0.0, 25.0, 50.0, 75.0, 100.0, 0.0, 25.0, 50.0, 75.0, 100.0 };

/*定义寄存器地址
1 - 归零
2 - XY位置
3 - 针头高度
*/
unsigned char ADDR[4] = { 0, 0, 0, 0 };
//Serial2正在读取
bool is_serial2_reading = false;
String frame2;                      //Serial2
unsigned long serial2_read_at = 0;  // 读取到串口数据的时间点
int serial2_read_interval = 500;
float pos_x = 0.0;  // `M114 R`读取到的XYZ位置
float pos_y = 0.0;
float pos_z = 0.0;

/*配置取样器*/
#define SAMPLE_Z_HIGH 15  //z轴升降的高度，单位mm

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  //初始化串口
  Serial.begin(baudrate);                         //调试用
  Serial1.begin(baudrate, SERIAL_8N1, RX1, TX1);  //RS485，和上位机通信
  Serial2.begin(115200);                          //TTL，和marlin通信
  Serial.println("# Marlin2SamplerWrapper v0.0.0");
  Serial.println("# by gu_jiefan@pharmablock.com");
  serial2_read_at = millis();
}

// called for each match
void match_callback(const char *match,          // matching string (not null-terminated)
                    const unsigned int length,  // length of matching string
                    const MatchState &ms)       // MatchState in use (to get captures)
{
  char cap[10];  // must be large enough to hold captures

  Serial.print("Matched: ");
  Serial.write((byte *)match, length);
  Serial.println();

  for (word i = 0; i < ms.level; i++) {
    Serial.print("Capture ");
    Serial.print(i, DEC);
    Serial.print(" = ");
    ms.GetCapture(cap, i);
    Serial.println(cap);
  }  // end of for each capture

}  // end of match_callback

void loop() {
  //延时
  unsigned int characterTime;
  //数据缓冲区定义
  unsigned char frame1[bufferSize];  //Serial1
  //接收到的数据原始CRC
  unsigned int receivedCrc;
  //接收到的数据长度
  unsigned char address1 = 0;
  unsigned char address2 = 0;
  //modbus通讯异常标识
  bool modbus_ok = false;
  delay(100);

  //延时1.5个字符宽度
  characterTime = 15000000 / baudrate;

  //如果串口缓冲区数据量大于0进入条件
  while (Serial1.available() > 0) {
    //接收的数据量应小于一帧数据的最大字节数量
    if (address1 < bufferSize) {
      frame1[address1] = Serial1.read();
      address1++;
    } else {
      //清空缓冲区
      Serial1.read();
    }
    //延迟
    delayMicroseconds(characterTime);
    //数据读取完成
    if (Serial1.available() == 0) {
      //在Serial输出从Serial1接收到的信息
      Serial.print("[HEX] ");
      Serial.println(hex_to_hex_string(frame1, address1));
      Serial.print("[TXT] ");
      Serial.write(frame1, address1);
      Serial.print("\n");

      //校验CRC
      unsigned short internalCrc = ModRTU_CRC((char *)frame1, address1 - 2);
      internalCrc >> 1;
      unsigned char high = internalCrc & 0xFF;
      unsigned char low = internalCrc >> 8;

      //校验通过
      if (low == frame1[address1 - 1] && high == frame1[address1 - 2]) {
        unsigned char slaveCode = frame1[0];
        //设备号匹配，兼容广播模式
        if (slaveCode == slaveID || slaveCode == 0) {
          //检查功能码
          unsigned char funcCode = frame1[1];
          if (funcCode == 6) {                                               //写入
            if (frame1[2] == 0x00 && frame1[3] <= 3 && frame1[4] == 0x00) {  //目前只支持这些功能，做一点简单的校验
              if (frame1[3] == 1)                                            //归零
              {
                Serial.println("G28");
                Serial2.println("G28");
                ADDR[1] = frame1[5];
                modbus_ok = true;
              } else if (frame1[3] == 2)  //移动XY位置
              {
                Serial.print("Move to ");
                Serial.println(frame1[5]);
                //                Serial2.printlnf("G0 X%d Y%d F6000", POINTS_X[frame1[5]], POINTS_Y[frame1[5]]);
                Serial2.print("G0 X");
                Serial2.print(POINTS_X[frame1[5]]);
                Serial2.print(" Y");
                Serial.print(POINTS_Y[frame1[5]]);
                Serial2.print(" F6000\n");
                ADDR[2] = frame1[5];
                modbus_ok = true;
              } else if (frame1[3] == 3)  //改变针头高度
              {
                Serial.print("Z position changed to - ");
                if (frame1[5] == 1) {
                  Serial.println("up");
                  ADDR[3] = frame1[5];
                  modbus_ok = true;
                } else if (frame1[5] == 2) {
                  ADDR[3] = frame1[5];
                  Serial.println("down");
                  modbus_ok = true;
                } else {
                  modbus_ok = false;
                }
                modbus_ok = false;
              }
            } else
              modbus_ok = false;
          }

          else if (funcCode == 3) {  // 读取
            //组装返回的数据
            frame1[5] = ADDR[frame1[3] - 10];
            if (frame1[3] == 11) {
              Serial.print("Read IS_Homed ");
            } else if (frame1[3] == 12) {
              Serial.print("Read XY ");
            } else if (frame1[3] == 13) {
              Serial.print("Read Z ");
            } else {
              Serial.print("Error ");
            }
            Serial.println(frame1[5]);
          }

          if (!modbus_ok) {
            frame1[1] += 0x80;
          }
          internalCrc = ModRTU_CRC((char *)frame1, address1 - 2);
          internalCrc >> 1;
          frame1[address1 - 2] = internalCrc & 0xFF;
          //     frame1[7] = internalCrc>>8;
          frame1[address1 - 1] = internalCrc >> 8;
          // Serial1.write(&frame1[0], 9);
          Serial1.write(&frame1[0], address1);
        }
      } else {
        //CRC校验失败，通过Serial返回报错信息，点亮LED
        Serial.println("^^^ CRC ERROR ^^^");
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }
  }

  // 读取Serial2，用于检查XYZ位置
  while (Serial2.available() > 0) {
    if (address2 < bufferSize) {
      frame2 += char(Serial2.read());
      address2++;
    } else {
      Serial2.read();
    }
    //延迟
    delayMicroseconds(characterTime);
    //数据读取完成
    if (Serial2.available() == 0) {
      Serial.print("frame2: ");
      Serial.println(frame2);
      Serial.print("address2: ");
      Serial.println(address2);
      if (frame2.endsWith("ok\n") && frame2.indexOf("X:") == 0) {
        // 解析XYZ的位置  X:0.00 Y:0.00 Z:0.00 E:0.00 Count A:0B:0 Z:0
        Serial.println("try to match");
        int index_of_y = frame2.indexOf(" Y:");
        String str_x;
        str_x = frame2.substring(2, index_of_y);
        pos_x = str_x.toFloat();

        int index_of_z = frame2.indexOf(" Z:");
        String str_y;
        str_y = frame2.substring(index_of_y + 3, index_of_z);
        pos_y = str_y.toFloat();

        int index_of_e = frame2.indexOf(" E:");
        String str_z;
        str_z = frame2.substring(index_of_z + 3, index_of_e);
        pos_z = str_z.toFloat();

        Serial.print("str_x: ");
        Serial.println(str_x);
        Serial.print("pos_x: ");
        Serial.println(pos_x);
        Serial.print("str_y: ");
        Serial.println(str_y);
        Serial.print("pos_y: ");
        Serial.println(pos_y);
        Serial.print("str_z: ");
        Serial.println(str_z);
        Serial.print("pos_z: ");
        Serial.println(pos_z);
      }
    }
  }
  frame2 = "";
  if (millis() - serial2_read_at > serial2_read_interval) {
    //要求marlin汇报当前位置
    Serial2.println("M114 R");
    serial2_read_at = millis();
  }
}

unsigned int ModRTU_CRC(char *buf, int len) {
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else
        crc >>= 1;
    }
  }
  return crc;
}

String hex_to_hex_string(unsigned char *_hex, int length) {
  String out = "";
  char _16[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  for (int i = 0; i < length; i++) {
    unsigned char n = _hex[i];
    int ii = n / 16;
    out += _16[ii];
    out += _16[n % 16];
    out += " ";
  }
  return out;
}
