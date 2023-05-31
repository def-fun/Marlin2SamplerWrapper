/*
ESP-WROOM-32开发板　－ 板载LED和串口测试程序
不知为何，用了原有的RX1-9和TX1-10会报错。修改管脚和Serial1的初始化方式后正常
https://github.com/espressif/arduino-esp32/issues/5463
烧录方法：
    1. USB直连开发板到电脑，或者通过USB2TTL工具连接开发板和电脑
    2. 使用Arduino IDE打开此程序，并配置ESP32开发环境，设置串口的端口
    3. 点击编译、上传的按钮
    4. 控制台显示`Connecting........__`的时候，按住开发板上的Boot按钮，再按下开发板上的EN按钮，松开两个按钮之后便开始烧录
    5. 烧录之后，按一下EN按钮以重启ESP32，之后便能看到ＬＥＤ闪烁，以及分别在三个串口接收到数据
*/

#include <HardwareSerial.h>
#define LED_BUILTIN 2
#define RX1 18
#define TX1 19
#define RX2 16
#define TX2 17


void setup() {
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8N1, RX1, TX1);
  Serial2.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  Serial.printf("seial-defalut\n");
  Serial1.printf("seial-1\n");
  Serial2.printf("seial-2\n");
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(500);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(500);
}
