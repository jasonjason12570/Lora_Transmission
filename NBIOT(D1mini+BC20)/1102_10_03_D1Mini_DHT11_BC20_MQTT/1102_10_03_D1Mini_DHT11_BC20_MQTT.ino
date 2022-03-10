#include <Arduino.h>
#include "BC20.h"
#include "MySQL.h"
//Extension
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

int pinDHT11 = 16;    // D0=16
byte temperature = 30.12;
byte humidity = 80.87;

//WIFI
#define WIFI_SSID "ISLab"          // WIFI SSID here
#define WIFI_PASSWORD "chlin33830" // WIFI password here
IPAddress server_addr(192, 168, 1, 200); // change to you server ip, note its form split by "," not "."

//MySQL
int MYSQLPort = 3306;                    //mysql port default is 3306
char user[] = "localuser";               // Your MySQL user login username(default is root),and note to change MYSQL user root can access from local to internet(%)
char pass[] = "qweasdzxc";               // Your MYSQL password
char ddb[] = "lora_local";

//
String MSG = "";
bool qurysearch_ok = false;
bool quryinsert_ok = false;
int nRow = 10;
int nCol = 3;
String quryArray[10][3];
String ACK = "2244";

WiFiClient client;
MySQL_Connection conn((Client *)&client);

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

  Serial.println("=====WIFI Communication Started=====");
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // try to connect with wifi
  Serial.print("WIFI Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP Address is : ");
  Serial.println(WiFi.localIP()); // print local IP address

  Serial.println("=====DB Communication Started=====");
  // connecting to the MySQL server
  Serial.println("DB - Connecting...");
  // try to connect to mysql server
  while (1)
  {
    if (conn.connect(server_addr, 3306, user, pass))
    {
      delay(1000);
      Serial.println("Connection true.");
      break;
    }
    else
    {
      Serial.print(".");
    }
  }
  delay(2000);
}

void loop() {
  // status:
  // 0(尚未收到Data)
  // 1(送出SYN,尚未收到新的ACK+SYN)
  // 2(收到ACK+SYN，送出ACK，尚未收到ACK)
  // 3(收到ACK，確認送出ACK，上傳Data中)
  Serial.println("=====================================");

  MQTTtopic = "TeamXX/NBIoT/Sensors";
  MQTTmessage ="{\"Temperature\":"+String(temperature)+",\"Humidity\":"+String(humidity)+"}";
  Publish_MQTT(MQTTtopic, MQTTmessage);
  

  Serial.println("PutTemp&Humi OK! ");
  delay(600*1000);      
}
