#include <Arduino.h>
#include "BC20.h"
//Extension
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

int pinDHT11 = 16;    // D0=16
byte temperature = 30.12;
byte humidity = 80.87;

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  delay(0.5*1000);

  Serial.println("Boot BC20 ");
  // 1. 啟動與設定連線的伺服器 
  BC20_initail();
  
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
  Serial.println("=====================================");

  MQTTtopic = "TeamXX/NBIoT/Sensors";
  MQTTmessage ="{\"Temperature\":"+String(temperature)+",\"Humidity\":"+String(humidity)+"}";
  Publish_MQTT(MQTTtopic, MQTTmessage);
  

  Serial.println("PutTemp&Humi OK! ");
  delay(600*1000);      
}
