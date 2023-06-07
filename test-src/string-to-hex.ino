/*
String -> hex
字符串转十六进制形式的字符串
*/
String test_str = "ABCD-1234-中文";

void setup(){
    Serial.begin(9600);
}

void loop(){
    delay(1000);
    Serial.println(string_to_hex_str(test_str));
    // 串口输出字符串414243442d313233342de4b8ade69687
}

String string_to_hex_str(String str){
    String _hex_str = "";
    for (int i=0; i< str.length(); i++){
        _hex_str += String(str[i], HEX);
    }
    return _hex_str;
}