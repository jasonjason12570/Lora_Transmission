
#include <SoftwareSerial.h>

SoftwareSerial mySerial(14, 12);    // (Rx=14=D5, Tx=12=D6) to BC20(Tx,Rx)

String data = "";                   // 儲存來自BC20回覆的字串參數

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
}

void loop() {

while (mySerial.available()) { // Check if there is an available byte to read,
  delay(10);                   // Delay added to make thing stable
  char c = mySerial.read();    // Conduct a serial read
  if (c == '\n') { break; }    // Exit the loop when the "" is detected after the word
  data += c;                   // Shorthand for data = data + c
}

if (data.length() > 0) {       // 判斷data內有值在更換
  data.trim();
  delay(100);
  Serial.println("Got Respone: ");  // 可刪除或註解掉
  Serial.println(data);
}
data = "";

while (Serial.available()) {   // Check if there is an available byte to read,
  delay(10);                   // Delay added to make thing stable
  char c = Serial.read();      // Conduct a serial read
  if (c == '\n') { break; }    // Exit the loop when the "" is detected after the word
  data += c;                   // Shorthand for data = data + c
}

if (data.length() > 0) {       // 判斷data內有值在更換
  data.trim();
  delay(100);
  Serial.print("Sending Command: "); // 可刪除或註解掉
  Serial.println(data);              // 可刪除或註解掉
  mySerial.println(data);
}
data = "";

}