#include <Arduino.h>
#include "BC20.h"
//#include "MySQL.h"
// Extension
#include <SimpleDHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
// D1 mini
int pinDHT11 = 16; // D0=16
SimpleDHT11 dht11(pinDHT11);
byte temperature = 0;
byte humidity = 0;
int err = SimpleDHTErrSuccess;

// WIFI
//#define WIFI_SSID "ISLab"                // WIFI SSID here
//#define WIFI_PASSWORD "chlin33830"       // WIFI password here
// IPAddress server_addr(192, 168, 1, 200); // change to you server ip, note its form split by "," not "."
#define WIFI_SSID "Owen"                  // WIFI SSID here
#define WIFI_PASSWORD "27368857"          // WIFI password here
IPAddress server_addr(192, 168, 31, 200); // change to you server ip, note its form split by "," not "."

// MySQL
int MYSQLPort = 3306; // mysql port default is 3306

char mysqluser[] = "localuser"; // Your MySQL user login username(default is root),and note to change MYSQL user root can access from local to internet(%)
char mysqlpass[] = "qweasdzxc"; // Your MYSQL password
char mysqlddb[] = "lora_local";
//
String MSG = "";
bool qurysearch_ok = false;
bool quryinsert_ok = false;
int nRow = 10;
int nCol = 3;
String quryArray[10][3];
String ACK = "2244";

int status = 0;
WiFiClient client;
MySQL_Connection conn((Client *)&client);
bool qurysearch()
{
  String tmp = "";
  int qrow = 0;
  bool getqury = false;
  char QUERY_SQL[] = "select * from lora_local.send where nodemcu_get ='0' LIMIT 1 ";

  // MYSQL checker
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  if (conn.connected())
  {
  }
  else
  {
    conn.close();
    // Serial.println("Connecting...");
    delay(200);
    if (conn.connect(server_addr, 3306, mysqluser, mysqlpass))
    {
      MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
      delay(500);
      Serial.println("Successful reconnect!");
    }
    else
    {
      Serial.println("Cannot reconnect!");
      return getqury;
    }
  }

  // Creat MySQL Connect Item
  cur_mem->execute(QUERY_SQL);
  // get columns
  column_names *cols = cur_mem->get_columns();
  row_values *row = NULL;
  do
  {
    row = cur_mem->get_next_row();
    if (row != NULL)
    {
      for (int f = 0; f < cols->num_fields; f++)
      {
        Serial.print(row->values[f]);
        quryArray[qrow][f] = row->values[f];
        if (f < cols->num_fields - 1)
        {
          Serial.print(',');
        }
      }
      qrow = qrow + 1;
      Serial.println();
      getqury = true;
    }
  } while (row != NULL);
  // 刪除mysql實例爲下次採集作準備
  delete cur_mem;

  return getqury;
}

void quryupdate(String updateID)
{
  String QUERY_SQL_update = "UPDATE lora_local.send SET nodemcu_get='1' WHERE id='" + updateID + "'";
  char Buf[500];
  QUERY_SQL_update.toCharArray(Buf, 500);
  Serial.println(Buf);

  // Creat MySQL Connect Item
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  if (conn.connected())
  {
    // do something your work
    cur_mem->execute(Buf);
    delete cur_mem;
    Serial.println("UPDATE success");
  }
  else
  {
    conn.close();
    Serial.println("Connecting...");
    delay(200); //
    if (conn.connect(server_addr, 3306, mysqluser, mysqlpass))
    {
      delay(500);
      Serial.println("Successful reconnect!");
      // Insert into DB
      Serial.println("UPDATE INTO DB");
      Serial.println(Buf);
      cur_mem->execute(Buf);
      delete cur_mem;
      Serial.println("UPDATE success");
    }
    else
    {
      Serial.println("Cannot reconnect! Drat.");
    }
  }
}

void setup()
{
  //////////D1-mini-BC20//////////
  Serial.begin(115200);

  mySerial.begin(9600);
  delay(0.5 * 1000);
  // BC20
  Serial.println("Boot BC20 ");

  BC20_initail();
  // BC20+MQTT
  delay(5 * 1000);
  int sta_pre = sta;

  build_MQTT_connect(server_IP, server_port);
  delay(5 * 1000);
  reading(sta_pre, sta);
  connect_MQTT();
  delay(5 * 1000);
  reading(sta_pre, sta);
  delay(10 * 1000);

  //////////D1-mini-Wifi//////////
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

  //////////D1-mini-DB//////////

  Serial.println("=====DB Communication Started=====");
  // connecting to the MySQL server
  Serial.println("DB - Connecting...");
  // try to connect to mysql server
  while (1)
  {
    if (conn.connect(server_addr, 3306, mysqluser, mysqlpass))
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

void loop()
{

  Serial.println("=====================================");
  Serial.println("Initial");

  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
  {
    Serial.print("Read DHT11 failed, err=");
    Serial.print(SimpleDHTErrCode(err));
    Serial.print(",");
    Serial.println(SimpleDHTErrDuration(err));
    delay(1 * 1000);
    return;
  }

  Serial.print("Get DHT11 data OK!! ");
  Serial.print(temperature);
  Serial.print(" *C, ");
  Serial.print(humidity);
  Serial.println(" %");

  SIMID = BC20_CIMI();
  Serial.println("SIMID = "+String(SIMID));
  IMEI = BC20_CGSN();
  Serial.println("IMEI = "+String(IMEI));

  
  MQTTtopic = "Forensics/Sensor";

  Serial.println(WiFi.localIP());

  /*
  if (qurysearch())
  {
    Serial.println("已完成Data擷取, 進入status 1");

    // 1(送出SYN,尚未收到新的ACK+SYN)
    status = 1;
    Serial.println("Success Qury");
    digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(1000);                     // wait for a second
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
    String MSG = quryArray[0][2];
    Serial.println(MSG);
    // status:
    // 0(尚未收到Data)
    // 1(送出SYN,尚未收到新的ACK+SYN)
    // 2(收到ACK+SYN，送出ACK，尚未收到ACK)
    // 3(收到ACK，確認送出ACK，上傳Data中)
    //===============Initial Data==================
    String msg_syn = "";
    String msg_ack_syn = "";
    String msg_ack = "";
    String msg_ack_flag = "1a";
    String msg_ack_validate = "";
    String msg_syn_validate = "";
    int ack_start = 0;
    int ack_end = 0;
    int len_response = 0;
    //=================================
    bool status_1 = true;
    bool status_2 = true;
    bool status_3 = true;
    while (status_1)
    {
      // Send SYN
      msg_syn = "[{ \"IMEI\":\"test123456789\" ,";
      msg_syn = msg_syn + "\"data\":\"" + String(MSG) + "\"," + "\"temperature\":\"" + String(temperature) + "\"," + "\"humidity\":\"" + String(humidity) + "\"}]";

      Publish_MQTT(MQTTtopic, msg_syn);
      Serial.println("BC20 sending...");
      Serial.println("msg_syn: " + msg_syn);
      delay(100 * 1000);
    }
  }
  else
  {
    Serial.println("Failed");
  }


  */
  delay(1000 * 1000);
}
