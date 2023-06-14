/*
ESP32+Arduino调试程序
fixme 目的是根据坐标判断瓶子位号，但是位号从1直接跳到25了，而且位号越来越大

Serial输出如下：
```
    Marlin2SamplerWrapper v0.0.0
    # by gu_jiefan@pharmablock.com
    [m] 1[REGISTER]
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00
    [12] 1
    [m] 25[REGISTER]
    00 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00 19 00 00 00
    [12] 25
    [m] 26[REGISTER]
    00 00 00 00 00 00 00 00 00 00 00 00 19 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00 1a 00 00 00
    [12] 26
    [m] 27[REGISTER]
    00 00 00 00 00 00 00 00 00 00 00 00 1a 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00 1b 00 00 00
    [12] 27
    [m] 28[REGISTER]
    00 00 00 00 00 00 00 00 00 00 00 00 1b 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00 1c 00 00 00
    [12] 28
    [m] 29[REGISTER]
    00 00 00 00 00 00 00 00 00 00 00 00 1c 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00 1d 00 00 00
    [12] 29
    [m] 30[REGISTER]
    00 00 00 00 00 00 00 00 00 00 00 00 1d 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00 1e 00 00 00
    [12] 30
```
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

/*提供样品盘参数，第一个瓶子的坐标在数组里的索引是2，以此类推. */
//#define SAMPLE_X_NUM 5        //x方向有5列
//#define SAMPLE_Y_NUM 5        //y方向有5列
//#define SAMPLE_POS_1_X 10     //第一个瓶子的x坐标
//#define SAMPLE_POS_1_Y 10     //第一个瓶子的y坐标
//#define SAMPLE_POS_END_X 110  //和第一个瓶子成对角线的瓶子的x坐标
//#define SAMPLE_POS_END_Y 110  //和第一个瓶子成对角线的瓶子的y坐标
#define COLLECTOR_POS_X 101.10  //第101个瓶子，废液瓶的x坐标
#define COLLECTOR_POS_Y 101.10  //第101个瓶子，废液瓶的y坐标
// __POINTS_X用于存储前25个瓶位的坐标，POINTS_X用于存储第101个瓶子的坐标
float __POINTS_X[] = { 1025.0, 0.0, -0.0, -0.0, -0.0, -0.0, 25.0, 25.0, 25.0, 25.0, 25.0, 50.0, 50.0, 50.0, 50.0, 50.0,
                       75.0, 75.0, 75.0, 75.0, 75.0, 100.0, 100.0, 100.0, 100.0, 100.0 };
float __POINTS_Y[] = { 1025.0, 0.0, 25.0, 50.0, 75.0, 100.0, 0.0, 25.0, 50.0, 75.0, 100.0, 0.0, 25.0, 50.0, 75.0, 100.0,
                       0.0, 25.0, 50.0, 75.0, 100.0, 0.0, 25.0, 50.0, 75.0, 100.0 };
float POINTS_X[128] = { 1025.0 };  // 所有大于1024的都是未定义的位置
float POINTS_Y[128] = { 1025.0 };
#define SAMPLE_Z_HIGH 15  //z轴升降的高度，单位mm

/* MODBUS寄存器地址，1-3为写入，11-13为读取，初始化均为0
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
int serial2_read_interval = 1200;   // 向marlin发送位置请求的时间间隔，单位毫秒
float pos_x = 0.0;                  // `M114 R`读取到的XYZ位置
float pos_y = 0.0;
float pos_z = 0.0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  //初始化串口
  Serial.begin(baudrate);                         //调试用
  Serial1.begin(baudrate, SERIAL_8N1, RX1, TX1);  //RS485，和上位机通信
  Serial2.begin(115200);                          //TTL，和marlin通信
  Serial.println("# Marlin2SamplerWrapper v0.0.0");
  Serial.println("# by gu_jiefan@pharmablock.com");
  serial2_read_at = millis();
  for (int i = 0; i < 25; i++) {
    POINTS_X[i] = __POINTS_X[i];
    POINTS_Y[i] = __POINTS_Y[i];
  }
  POINTS_X[101] = COLLECTOR_POS_X;
  POINTS_Y[101] = COLLECTOR_POS_Y;
}

String hex_to_hex_string(unsigned char *_hex, int length);

unsigned int ModRTU_CRC(char *buf, int len);


void loop_fake(String m114_resp) {
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


  // 读取Serial2，用于检查XYZ位置
  while (m114_resp.length() > 0) {
    if (address2 < bufferSize) {
      frame2 += m114_resp[0];
      address2++;
      m114_resp = m114_resp.substring(1, m114_resp.length());
      // Serial.println(m114_resp);
    } else {
      m114_resp.clear();
    }
    // Serial.print("m114.len ");
    // Serial.println(m114_resp.length());
  }

  //数据读取完成
  if (m114_resp.length() == 0) {

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

      for (int m = 0; m < 127; m++) {  // 查找XY坐标和编号的关系
        float delta_x;
        float delta_y;
        delta_x = POINTS_X[m] - pos_x;
        delta_y = POINTS_Y[m] - pos_y;
        if (abs(delta_x) < 0.03 && abs(delta_y) < 0.03) {
          Serial.print("[m] ");
          Serial.print(m);
          Serial.println("[REGISTER] ");
          Serial.println(hex_to_hex_string(REGISTER, 16));
          REGISTER[12] = (unsigned char)m;
          Serial.println(hex_to_hex_string(REGISTER, 16));
          Serial.print("[12] ");
          Serial.println((int)REGISTER[12]);
        }
      }
      if (-0.1 < (pos_z - SAMPLE_Z_HIGH) < 0.1) {
        REGISTER[13] = 0x02;
      } else if (-0.1 < pos_z < 0.1) {
        REGISTER[13] = 0x01;
      } else {
        REGISTER[13] = 0;
      }
    }
  }
  frame2 = "";
  Serial.print("---------");
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

void loop() {
  loop_fake("X:0.01 Y:0.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  loop_fake("X:0.01 Y:0.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  loop_fake("X:-0.01 Y:25.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:-0.01 Y:50.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:-0.01 Y:75.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:-0.01 Y:100.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:25.01 Y:0.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:25.01 Y:25.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:25.01 Y:50.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:25.01 Y:75.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:25.01 Y:100.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:50.01 Y:0.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:50.01 Y:25.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:50.01 Y:50.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:50.01 Y:75.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:50.01 Y:100.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:75.01 Y:0.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:75.01 Y:25.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:75.01 Y:50.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:75.01 Y:75.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:75.01 Y:100.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:100.01 Y:0.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:100.01 Y:25.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:100.01 Y:50.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:100.01 Y:75.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  // loop_fake("X:100.01 Y:100.0 Z:0.00 E:0.00 Count A:0B:0 Z:0\nok\n");
  delay(1000);
}