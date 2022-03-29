#include <Arduino.h>
#include <SimpleDHT.h>

#include "BC20.h"

int pinDHT11 = 16;    // D0=16
SimpleDHT11 dht11(pinDHT11);
byte temperature = 0;
byte humidity = 0;
int err = SimpleDHTErrSuccess;

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  delay(0.5*1000);

  Serial.println("Boot BC20 ");
  BC20_initail(); // 啟動與設定連線的伺服器  
  delay(5*1000);
  int sta_pre = sta;
  build_MQTT_connect(server_IP, server_port);
  delay(5*1000);
  reading(sta_pre, sta);
  connect_MQTT();
  delay(5*1000);
  reading(sta_pre, sta);
  delay(10*1000);
}

void loop() {
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1*1000);
    return;
  }
  
  Serial.print("Get DHT11 data OK!! ");
  Serial.print(temperature); Serial.print(" *C, "); 
  Serial.print(humidity); Serial.println(" %");

  MQTTtopic = "TeamXX/NBIoT/Temp";
  MQTTmessage = String(temperature);
  Publish_MQTT(MQTTtopic, MQTTmessage);
  delay(10*1000);

  MQTTtopic = "TeamXX/NBIoT/Humi";
  MQTTmessage = String(humidity);
  Publish_MQTT(MQTTtopic, MQTTmessage);
  delay(10*1000);

  MQTTtopic = "TeamXX/NBIoT/Sensors";
  MQTTmessage ="{\"Temperature\":"+String(temperature)+",\"Humidity\":"+String(humidity)+"}";
  Publish_MQTT(MQTTtopic, MQTTmessage);
  delay(10*1000);

  Serial.println("PutTemp&Humi OK! ");
  delay(5*1000);      
}
