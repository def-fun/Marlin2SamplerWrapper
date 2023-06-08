/*
Arduino使用正则表达式+
https://www.cnblogs.com/dapenson/p/Regex_Arduino.html
*/

#include <Arduino.h>
#include <Regexp.h>  //第三方库，可通过Arduino IDE安装  https://github.com/nickgammon/Regexp

// called for each match
void match_callback(const char *match,         // matching string (not null-terminated)
                    const unsigned int length, // length of matching string
                    const MatchState &ms)      // MatchState in use (to get captures)
{
  char cap[10]; // must be large enough to hold captures

  Serial.print("Matched: ");
  Serial.write((byte *)match, length);
  Serial.println();

  for (word i = 0; i < ms.level; i++)
  {
    Serial.print("Capture ");
    Serial.print(i, DEC);
    Serial.print(" = ");
    ms.GetCapture(cap, i);
    Serial.println(cap);
  } // end of for each capture

} // end of match_callback

void testFunc()
{
  unsigned long count;

  // what we are searching (the target)
  char buf[100] = "content:a1.1 bb22.22ccc333.333dddd4444.666 ";

  // match state object
  MatchState ms(buf);

  // original buffer
  Serial.println(buf);

  // search for three letters followed by a space (two captures)
  count = ms.GlobalMatch("(%a+)(%d+%.%d+)", match_callback);

  // show results
  Serial.print("Found ");
  Serial.print(count); // 8 in this case
  Serial.println(" matches.");
}

void setup()
{
  Serial.begin(9600);
  Serial.println();

} // end of setup

void loop()
{
  Serial.printf("\n\n");
  testFunc();
  delay(5 * 1000);
}