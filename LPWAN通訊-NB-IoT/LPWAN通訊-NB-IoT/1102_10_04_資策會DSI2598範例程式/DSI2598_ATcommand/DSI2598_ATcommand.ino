#include <SoftwareSerial.h>
SoftwareSerial mySerial(8, 9);

String data = "";
String data2 = "";
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  mySerial.begin(9600);
  Serial.println("begin...");
}

int a = 0;
int b=0;
void loop() {
  // put your main code here, to run repeatedly:
  int got=0;
  while(Serial.available()) {
    char c = Serial.read();
    data += c;
    got++;
  }
  if(data.length() > 0) {
    //Serial.println(data);
    delay(1);
    if(got) {
    } else {
      a++;
      if(a<=10) {
        //Serial.println("one more");
        //delay(10);
      } else {

        data.trim();
        Serial.println("sending command");
        Serial.println(data);
        mySerial.println(data);
        a=0;
        data="";
      }
    }  
  }

  int got2=0;
  while(mySerial.available()) {
    char c = mySerial.read();
    data2 += c;
    got2++;
  }
  if(data2.length() > 0) {
    delay(1);
    //Serial.println(data2);
    if(got2) {
    } else {
      b++;
      if(b<=10) {
        //Serial.println("one more 2");
        //delay(10);
      } else {
        Serial.println("got result");
        Serial.println(data2);
        b=0;
        data2="";
      }
    }  
  }

  
 
}
