/*
Arduino Modbus 从站
https://www.heibing.org/2019/12/136
*/
#define bufferSize 255      //一帧数据的最大字节数量
#define baudrate 9600       //定义通讯波特率
#define slaveID 20          //定义modbus RTU从站站号，20 == 0x14
#define modbusDataSize 100  //定义modbus数据库空间大小
#define LED_BUILTIN 2
#define RX1 18
#define TX1 19
#define RX2 16
#define TX2 17

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  //定义串口
  Serial.begin(baudrate);                         //调试用
  Serial1.begin(baudrate, SERIAL_8N1, RX1, TX1);  //RS485，和上位机通信
  Serial2.begin(baudrate);                        //TTL，和marlin通信
  Serial.println("# Marlin2SamplerWrapper v0.0");
  Serial.println("# by gu_jiefan @20230601");
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
          if (funcCode == 3) {
            //组装返回的数据
            frame[2] = 0x04;
            frame[3] = 0x00;
            frame[4] = 0x00;
            frame[5] = 0x00;
            frame[6] = val;
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

      internalCrc = ModRTU_CRC((char*)frame, 7);
      internalCrc >> 1;
      frame[7] = internalCrc & 0xFF;
      //     frame[7] = internalCrc>>8;
      frame[8] = internalCrc >> 8;
      Serial1.write(&frame[0], 9);
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