/*
3d打印机作为自动取样器（Modbus从站），基于ESP32和Arduino IDE
todo 自动计算所有样品瓶的坐标
todo 检查G28的返回值之后，再返回`已归零`
TODO
feature 瓶号不在定义范围内的，会丢弃命令
feature 完整支持modbus RTU的03功能码，如`14 03 00 0B 00 03 76 CC`  -> `14 03 06 00 02 00 03 00 04 5B E6`
reference https://www.heibing.org/2019/12/136
*/
#include <Arduino.h>

/*通讯参数*/
#define bufferSize 255      //modbus一帧数据的最大字节数量
#define SERIAL0_BAUDRATE 9600       //定义默认串口的波特率
#define SERIAL1_BAUDRATE 9600       //定义modbus通讯的波特率
#define SERIAL2_BAUDRATE 115200     //定义和marlin通讯的波特率
#define slaveID 20          //定义modbus RTU从站站号，20 == 0x14
#define modbusDataSize 100  //定义modbus数据库空间大小
#define LED_BUILTIN 2       //板载LED
#define RX1 18              //Serial1，转RS485 modbus和上位机通信
#define TX1 19
#define RX2 16              //Serial2，TTL，和marlin通信
#define TX2 17

/*提供样品盘参数，第一个瓶子的坐标排在数组里的第二个（下标1），以此类推. */
#define SAMPLE_X_NUM 5  //x方向有5列
#define SAMPLE_Y_NUM 5  //y方向有5列
//#define SAMPLE_POS_1_X 10     //第一个瓶子的x坐标
//#define SAMPLE_POS_1_Y 10     //第一个瓶子的y坐标
//#define SAMPLE_POS_END_X 110  //和第一个瓶子成对角线的瓶子的x坐标
//#define SAMPLE_POS_END_Y 110  //和第一个瓶子成对角线的瓶子的y坐标
#define COLLECTOR_POS_X 130.10  //第101个瓶子，废液瓶的x坐标
#define COLLECTOR_POS_Y 101.10  //第101个瓶子，废液瓶的y坐标
#define COLLECTOR_POS 101       //废液瓶的编号
// __POINTS_X用于存储前25个瓶位的坐标，POINTS_X用于存储127个瓶子的坐标（索引0不计入）
float __POINTS_X[] = { 0.0, 10.0, 10.0, 10.0, 10.0, 10.0, 35.0, 35.0, 35.0, 35.0, 35.0, 60.0, 60.0, 60.0, 60.0, 60.0, 85.0, 85.0, 85.0, 85.0, 85.0, 110.0, 110.0, 110.0, 110.0, 110.0 };
float __POINTS_Y[] = { 0.0, 10.0, 35.0, 60.0, 85.0, 110.0, 10.0, 35.0, 60.0, 85.0, 110.0, 10.0, 35.0, 60.0, 85.0, 110.0, 10.0, 35.0, 60.0, 85.0, 110.0, 10.0, 35.0, 60.0, 85.0, 110.0 };
// 后面用1025.0初始化所有元素，因此所有大于1024的坐标都是错误的、不在考虑范围内的位置
float POINTS_X[128];
float POINTS_Y[128];
#define SAMPLE_Z_HIGH 15.0  //z轴升降的高度，单位mm
int XY_LIMIT = SAMPLE_X_NUM * SAMPLE_Y_NUM;
bool is_homed = false; //通过原地移动来判断是否已经归零
int ok_count = 0;  //统计收到ok回复的个数

/* MODBUS寄存器地址，1-3存储写入的值，11-13存储当前的状态，初始化均为0
    1,11 - 是否归零，0x00 没有归零，0x01 XYZ三轴已经归零
    2,12 - XY位置，1-25为取样瓶，101为废液瓶
    3,13 - 针头升降，0x01 针头抬起，0x02 针头落下
*/
unsigned char REGISTER[16];
unsigned char WRITE_CODE = 0x06;
unsigned char READ_CODE = 0x03;

//Serial2正在读取
bool is_serial2_reading = false;
String frame2 = "";                 //Serial2
unsigned long serial2_read_at = 0;  // 读取到串口数据的时间点
int SERIAL2_READ_INTERVAL = 1200;   // 向marlin发送位置请求的时间间隔，单位毫秒
float pos_x = 0.0;                  // `M114 R`读取到的XYZ位置
float pos_y = 0.0;
float pos_z = 0.0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  //初始化串口
  Serial.begin(SERIAL0_BAUDRATE);                         //调试用
  Serial1.begin(SERIAL1_BAUDRATE, SERIAL_8N1, RX1, TX1);  //RS485，和上位机通信
  Serial2.begin(SERIAL2_BAUDRATE);                          //TTL，和marlin通信
  Serial.println("# Marlin2SamplerWrapper v0.0.0");
  Serial.println("# by gu_jiefan@pharmablock.com");
  serial2_read_at = millis();
  for (int i = 0; i < 128; i++) {  //数组里的所有元素用0.0初始化
    POINTS_X[i] = 0.0;
    POINTS_Y[i] = 0.0;
  }
  for (int i = 0; i <= 25; i++) {
    POINTS_X[i] = __POINTS_X[i];
    POINTS_Y[i] = __POINTS_Y[i];
  }
  POINTS_X[101] = COLLECTOR_POS_X;
  POINTS_Y[101] = COLLECTOR_POS_Y;
  // 打印出所有瓶位和对应的XY坐标
  // for(int i=0; i<128; i++){
  //   Serial.print(i);
  //   Serial.print(" ");
  //   Serial.print(POINTS_X[i]);
  //   Serial.print(", ");
  //   Serial.println(POINTS_Y[i]);
  // }
  // Serial.println("------------------------------");
}

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
  characterTime = 15000000 / SERIAL1_BAUDRATE;

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
      //      Serial.print("[HEX] ");
      //      Serial.println(hex_to_hex_string(frame1, address1));
      //      Serial.print("[TXT] ");
      //      Serial.write(frame1, address1);
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
          //通讯成功，熄灭板载LED
          digitalWrite(LED_BUILTIN, LOW);
          unsigned char funcCode = frame1[1];
          unsigned char register_addr = frame1[2] * 256 + frame1[3];
          unsigned char write_payload = frame1[4] * 256 + frame1[5];  //可能会溢出
          //校验范围
          if (funcCode != WRITE_CODE && funcCode != READ_CODE) { break; }
          if (register_addr > 15) { break; }
          if (write_payload > 128) { break; }

          if (funcCode == WRITE_CODE) {   //写入
            if (register_addr == 0x01) {  // 归零
              Serial.println("Homing");   // XYZ三轴归零
              if (write_payload == 0x01) {
                Serial2.println("G28");
                REGISTER[0x01] = write_payload;
                REGISTER[11] = 1;               //表明已经归零
              } else if (write_payload == 0x02) {  //XY归零
                Serial2.println("G28 X Y");
                REGISTER[0x01] = write_payload;
                REGISTER[11] = 1;  //表明已经归零
              }
              modbus_ok = true;
            } else if (register_addr == 0x02) {  //移动XY位置
              Serial.print("Move to pos: ");
              Serial.println(write_payload);
              REGISTER[0x02] = write_payload;
              if (write_payload > XY_LIMIT && write_payload != COLLECTOR_POS) {
                Serial.print("Out of limit");
                modbus_ok = false;
              } else {
                //Serial2.printlnf("G0 X%d Y%d F6000", POINTS_X[frame1[5]], POINTS_Y[frame1[5]]);
                Serial2.print("G0 X");
                Serial2.print(POINTS_X[write_payload]);
                Serial2.print(" Y");
                Serial2.print(POINTS_Y[write_payload]);
                Serial2.print(" F3000\n");
                modbus_ok = true;
              }
            } else if (register_addr == 0x03) {  //改变针头高度，速度太快会丢步
              if (write_payload == 0x01) {       // 针头抬起
                Serial.println("Z up");
                Serial2.print("G0 Z0 F800\n");
                REGISTER[0x03] = 0x01;
                REGISTER[13] = 0x01;
                modbus_ok = true;
              } else if (write_payload == 0x02) {  //针头下降
                Serial.println("Z down");
                Serial2.print("G0 Z");
                Serial2.print(SAMPLE_Z_HIGH);
                Serial2.print(" F800\n");
                REGISTER[0x03] = 0x02;
                REGISTER[13] = 0x02;
                modbus_ok = true;
              } else {
                modbus_ok = false;
              }
            } else {
              modbus_ok = false;
            }
          } else if (funcCode == READ_CODE) {     // 读取
            frame1[5] = REGISTER[register_addr];  // 大于256会溢出
            if (register_addr == 11) {
              Serial.print("Read IS_Homed: ");
            } else if (register_addr == 12) {
              Serial.print("Read XY: ");
            } else if (register_addr == 13) {
              Serial.print("Read Z: ");
            } else {
              Serial.print("Error ");
            }
            frame1[2] = 2 * write_payload;     //MODBUS有效载荷的字节数
            address1 = 5 + 2 * write_payload;  //MODBUS响应的总字节数，包括1字节站号、1字节功能码、1字节有效载荷长度、载荷和2字节CRC校验
            for (int i = 1; i <= write_payload; i++) {
              frame1[2 * i + 1] = 0;
              frame1[2 * i + 2] = REGISTER[register_addr + i - 1];
            }
            Serial.println(REGISTER[register_addr]);  // register_addr大于16会溢出
            Serial.print("[REGISTER HEX] ");
            Serial.println(hex_to_hex_string(REGISTER, 16));
            modbus_ok = true;
          } else {
            modbus_ok = false;
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
      frame2.clear();
    }
    //延迟
    delayMicroseconds(characterTime);
    //数据读取完成
    if (Serial2.available() == 0) {
      //      Serial.print("frame2: ");
      //      Serial.println(frame2);
      //      Serial.print("address2: ");
      //      Serial.println(address2);
      // echo:Home X First
      if(frame2.indexOf("Home")>0 && frame2.indexOf("First") > 0){
         //需要归零
         REGISTER[11] = 0;
      }
      if (frame2.endsWith("ok\n") && frame2.indexOf("X:") == 0) {
        // 解析XYZ的位置  X:0.00 Y:0.00 Z:0.00 E:0.00 Count A:0B:0 Z:0
        //        Serial.println("try to match");
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

        //        Serial.print("str_x: ");
        //        Serial.println(str_x);
        //        Serial.print("pos_x: ");
        //        Serial.println(pos_x);
        //        Serial.print("str_y: ");
        //        Serial.println(str_y);
        //        Serial.print("pos_y: ");
        //        Serial.println(pos_y);
        //        Serial.print("str_z: ");
        //        Serial.println(str_z);
        //        Serial.print("pos_z: ");


        for (int m = 0; m < 127; m++) {  // 查找XY坐标和编号的关系
          float delta_x;
          float delta_y;
          delta_x = POINTS_X[m] - pos_x;
          delta_y = POINTS_Y[m] - pos_y;
          if (abs(delta_x) < 0.03 && abs(delta_y) < 0.03) {
            //          Serial.print("[m] ");
            //          Serial.print(m);
            //          Serial.println("[REGISTER] ");
            //          Serial.println(hex_to_hex_string(REGISTER, 16));
            REGISTER[12] = (unsigned char)m;
            //          Serial.println(hex_to_hex_string(REGISTER, 16));
            //          Serial.print("[12] ");
            //          Serial.println((int)REGISTER[12]);
          }
        }
        float delta_z;
        delta_z = pos_z - SAMPLE_Z_HIGH;
        if (abs(delta_z) < 0.1) {
          REGISTER[13] = 0x02;
        } else if (abs(pos_z) < 0.1) {
          REGISTER[13] = 0x01;
        } else {
          REGISTER[13] = 0;
        }

        Serial.print("[POS] ");
        Serial.print(pos_x);
        Serial.print(" ");
        Serial.print(pos_y);
        Serial.print(" ");
        Serial.println(pos_z);
      }
    }
  }
  frame2 = "";
  if (millis() - serial2_read_at > SERIAL2_READ_INTERVAL) {
    //固定每SERIAL2_READ_INTERVAL毫秒向marlin查询一次当前位置
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
