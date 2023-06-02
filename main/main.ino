/*
Arduino Modbus 从站
todo 自动计算所有样品瓶的坐标
todo 检查marlin返回的数据之后，再响应modbus
https://www.heibing.org/2019/12/136
*/
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

/*配置取样器*/
#define SAMPLE_Z_HIGH 15  //z轴升降的高度，单位mm

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  //初始化串口
  Serial.begin(baudrate);                         //调试用
  Serial1.begin(baudrate, SERIAL_8N1, RX1, TX1);  //RS485，和上位机通信
  Serial2.begin(baudrate);                        //TTL，和marlin通信
  Serial.println("# Marlin2SamplerWrapper v0.0.0");
  Serial.println("# by gu_jiefan@pharmablock.com");
}

void loop() {
  //延时
  unsigned int characterTime;
  //数据缓冲区定义
  unsigned char frame[bufferSize];

  //接收到的数据原始CRC
  unsigned int receivedCrc;
  //接收到的数据长度
  unsigned char address = 0;
  //modbus通讯异常标识
  bool modbus_ok = false;
  int val;
  val = analogRead(0);
  //Serial1.println(val,DEC);
  delay(100);

  //延时1.5个字符宽度
  characterTime = 15000000 / baudrate;

  //如果串口缓冲区数据量大于0进入条件
  while (Serial1.available() > 0) {
    //接收的数据量应小于一帧数据的最大字节数量
    if (address < bufferSize) {
      frame[address] = Serial1.read();
      address++;
    } else {
      //清空缓冲区
      Serial1.read();
    }
    //延迟
    delayMicroseconds(characterTime);
    //数据读取完成
    if (Serial1.available() == 0) {
      //校验CRC
      unsigned short internalCrc = ModRTU_CRC((char*)frame, address - 2);
      internalCrc >> 1;
      unsigned char high = internalCrc & 0xFF;
      unsigned char low = internalCrc >> 8;

      //校验通过
      if (low == frame[address - 1] && high == frame[address - 2]) {
        unsigned char slaveCode = frame[0];
        //设备号匹配，兼容广播模式
        if (slaveCode == slaveID || slaveCode == 0) {
          //检查功能码
          unsigned char funcCode = frame[1];
          if (funcCode == 6) {
            if (frame[2] == 0x00 && frame[3] == 0x01) {  //取样器回零点
              if (frame[4] == 0x00 && frame[5] == 0x01)  //XYZ三轴归零
              {
                Serial.println("G28");
                Serial2.println("G28");
                modbus_ok = true;
              } else if (frame[4] == 0x00 && frame[5] == 0x02)  //X轴归零
              {
                Serial.println("G28 X");
                Serial2.println("G28 X");
                modbus_ok = true;
              } else if (frame[4] == 0x00 && frame[5] == 0x03)  //Y轴归零
              {
                Serial.println("G28 Y");
                Serial2.println("G28 Y");
                modbus_ok = true;
              } else if (frame[4] == 0x00 && frame[5] == 0x04)  //Z轴归零
              {
                Serial.println("G28 Z");
                Serial2.println("G28 Z");
                modbus_ok = true;
              } else {
                Serial.println("Not allowed payload");
                Serial.write(frame, address);
                modbus_ok = false;
              }
            } else if (frame[2] == 0x00 && frame[3] == 0x06) {  // 移动并注射
              if (frame[4] == 0x00 && frame[5] == 0x01)         //移动到第1个瓶子
              {
                Serial.println("G0 Z1");
                Serial.println("G0 X10 Y10");
                Serial.println("G0 Z15");
                Serial2.println("G0 Z1");
                Serial2.println("G0 X10 Y10");
                Serial2.println("G0 Z15");
                modbus_ok = true;
              } else if (frame[4] == 0x00 && frame[5] == 0x02)  //X轴归零
              {
                Serial.println("G0 Z1");
                Serial.println("G0 X10 Y20");
                Serial.println("G0 Z15");
                Serial2.println("G0 Z1");
                Serial2.println("G0 X10 Y20");
                Serial2.println("G0 Z15");
                modbus_ok = true;
              } else if (frame[4] == 0x00 && frame[5] == 0x03)  //Y轴归零
              {
                Serial.println("G0 Z1");
                Serial.println("G0 X10 Y30");
                Serial.println("G0 Z15");
                Serial2.println("G0 Z1");
                Serial2.println("G0 X10 Y30");
                Serial2.println("G0 Z15");
                modbus_ok = true;
              } else if (frame[4] == 0x00 && frame[5] == 0x04)  //Z轴归零
              {
                Serial.println("G0 Z1");
                Serial.println("G0 X10 Y40");
                Serial.println("G0 Z15");
                Serial2.println("G0 Z1");
                Serial2.println("G0 X10 Y40");
                Serial2.println("G0 Z15");
                modbus_ok = true;
              } else {
                Serial.println("Not allowed payload");
                Serial.write(frame, address);
                modbus_ok = false;
              }
            }
            // frame[2] = 0x04;
            // frame[3] = 0x00;
            // frame[4] = 0x00;
            // frame[5] = 0x00;
            // frame[6] = val;
          }

          else {
            //组装返回的数据
            frame[2] = 0x00;
            frame[3] = 0x00;
            frame[4] = 0x00;
            //报错：未知功能码

            frame[5] = 0x01;
          }
        }
      }

      else {
        //CRC校验失败，通过Serial返回报错信息，点亮LED
        Serial.println("CRC error");
        Serial.write(frame, address);
        digitalWrite(LED_BUILTIN, HIGH);
      }
      if (!modbus_ok) {
        frame[1] += 0x80;
      }
      internalCrc = ModRTU_CRC((char*)frame, address - 2);
      internalCrc >> 1;
      frame[address - 2] = internalCrc & 0xFF;
      //     frame[7] = internalCrc>>8;
      frame[address - 1] = internalCrc >> 8;
      // Serial1.write(&frame[0], 9);
      Serial1.write(&frame[0], address);
    }
  }
}

unsigned int ModRTU_CRC(char* buf, int len) {
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