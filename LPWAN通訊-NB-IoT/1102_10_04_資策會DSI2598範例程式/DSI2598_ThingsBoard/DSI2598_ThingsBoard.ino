#include "BC26.h"
#include "ideaschain.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  mySerial.begin(9600);
  delay(500);
  Serial.println("Boot BC26 ");
  BC26_initail();               //  啟動與設定連線的伺服器
  delay(2000);
  data_publish(topic, message(String(reset_count)));
  delay(2000);
}

void loop() {
  // put your main code here, to run repeatedly:  
  if(reset_count >= 3600){     //  30min後系統重開
    resetFunc();
  }
  if(time_count >= 60){
    data_publish(topic, message(String(reset_count)));
    time_count = 0;
  }
  delay(500);
  time_count++;
  reset_count++;              //  系統重新啟動計時器累加
}
