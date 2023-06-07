/*
以十六进制字符串的形式打印出hex数据，用于在控制台查看hex数据
如：unsigned char test_hex[] = { 0x41, 0x42, 0x43, 0x44, 0x0a, 0x0b, 0x0c, 0x0d };
   将打印出字符串`41 42 43 44 0a 0b 0c 0d`
*/
unsigned char test_hex[] = { 0x41, 0x42, 0x43, 0x44, 0x0a, 0x0b, 0x0c, 0x0d };

void setup() {
  Serial.begin(9600);
}

void loop() {
  delay(1000);
  Serial.print("hex_to_hex_string: ");
  Serial.println(hex_to_hex_string(test_hex, 8));
  Serial.print("hex_to_hex_char: ");
  int length = 8;
  char out[length * 3 + 1];
  hex_to_hex_char(test_hex, length, out);
  Serial.println(out);
  /*  串口输出
    hex_to_hex_string: 41 42 43 44 0a 0b 0c 0d
    hex_to_hex_char: 41 42 43 44 0a 0b 0c 0d
  */
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

void hex_to_hex_char(unsigned char *_hex, int length, char *out) {
  char _16[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  for (int i = 0; i < length; i++) {
    unsigned char n = _hex[i];
    int ii = n / 16;
    out[i * 3] = _16[ii];
    out[i * 3 + 1] = _16[n % 16];
    out[i * 3 + 2] = ' ';
  }
  out[3 * length] = '\0';
}