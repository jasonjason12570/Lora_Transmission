#include <SoftwareSerial.h>

SoftwareSerial mySerial(14, 12); // (Rx=14=D5, Tx=12=D6) to BC20(Tx,Rx)

String data = "";    // 儲存來自BC20回覆的字串參數
String data_on = ""; // 擷取部分字串參數的儲存參數
int sta = 1;         // 迴圈的status參數判斷
int time_count = 0;
int count = 0; // 副程式中來自BC20的回覆次數計數器
int band = 8;  // 設定頻段(B8)
int IP_count1 = 0;
void (*resetFunc)(void) = 0; // 宣告系統重置參數
int reset_count = 0;         // 系統重新啟動計時器宣告
String data_tempt = "";
String SIMID = "";
String IMEI = "";

String server_IP = "140.128.99.71";
String server_port = "1883";
String clientID = "\"D1mini-qweasdzxc312"; // 請修改成唯一值，可用亂數
String user = "\"thulora\"";
String pass = "\"hpclab99\"";
String noreply = "no reply, reset all";
String MQTTtopic;
String MQTTmessage;

String BC20_CIMI()
{ // Get Sim 卡號
  Serial.println("BC20_CIMI: AT+CIMI");
  mySerial.println("AT+CIMI");
  sta = 5;
  int check = 1;
  while (sta == 5)
  { // 等待模組訊息回覆
    while (mySerial.available())
    {                           // Check if there is an available byte to read,
      delay(10);                // Delay added to make thing stable
      char c = mySerial.read(); // Conduct a serial read

      if (data == "AT+CIMI" && check == 1)
      {
        data = "";
        check = 2;
        Serial.println("---check1---");
      }
      else if (c == '\n' && check == 2)
      {
        Serial.println("data = " + data);
        Serial.println("---check2---");
        break;
      }

      data += c; // Shorthand for data = data + c
    }
    if (data.length() > 0)
    { // 判斷data內有值在更換
      data.trim();
      delay(100);

      // Serial.println(data);
      sta = 0;
    }
    delay(1 * 1000);
    count++;
    if (count > 10)
    { // 超過10秒未有回覆，重新啟動系統
      count = 0;
      Serial.println(noreply);
      // resetFunc();
    }
  }
  count = 0;
  delay(5 * 1000);
  return data;
}

String BC20_CGSN()
{ // Get IMEI 卡號
  Serial.println("BC20_CGSN: AT+CGSN");
  mySerial.println("AT+CGSN=1");
  sta = 6;
  data = "";
  int check = 1;
  while (sta == 6)
  { // 等待模組訊息回覆
    while (mySerial.available())
    {                           // Check if there is an available byte to read,
      delay(10);                // Delay added to make thing stable
      char c = mySerial.read(); // Conduct a serial read
      if (data == "+CGSN: " && check == 1)
      {
        data = "";
        check = 2;
      }
      else if (c == '\n' && check == 2)
      {
        break;
      }
      data += c; // Shorthand for data = data + c
    }
    if (data.length() > 0)
    { // 判斷data內有值在更換
      data.trim();
      delay(100);
      // Serial.println(data);
      sta = 0;
    }
    delay(1 * 1000);
    count++;
    if (count > 10)
    { // 超過10秒未有回覆，重新啟動系統
      count = 0;
      Serial.println(noreply);
      // resetFunc();
    }
  }
  count = 0;
  data = "";
  delay(5 * 1000);
  return data;
}

void serial_read()
{
  while (mySerial.available())
  {                           // Check if there is an available byte to read,
    delay(10);                // Delay added to make thing stable
    char c = mySerial.read(); // Conduct a serial read
    if (c == '\n')
    {
      break; // Exit the loop when the "" is detected after the word
    }
    data += c; // Shorthand for data = data + c
  }

  if (data.length() > 0)
  { // 判斷data內有值在更換
    data.trim();
    delay(100);
    Serial.println(data);
    if (sta == 1)
    {
      count++;
    }

    if (sta == 1)
    {
      if (count >= 1)
      { // Turn on BC20 status
        sta++;
        count = 0;
      }
    }
    else if (sta == 2)
    {
      if (data == "OK")
      { // 設置頻段
        sta++;
      }
    }
    else if (sta == 3)
    { // wait for IP status
      for (int i = 0; i < 3; i++)
      {
        data_on += data[i];
      }
      if (data_on == "+IP")
      {
        data_tempt = data_on;
        sta++;
      }
    }
    else if (sta == 4)
    {
      if (data == "OK")
      { // 連線至MQTT平台伺服器
        sta++;
      }
    }
    else if (sta == 5)
    {
      if (data == "OK")
      { // 連線至自己的頻道
        sta++;
      }
    }
    else if (sta == 6)
    { // 傳輸資料與伺服器端訊息回傳
      // +QSONMI
      for (int i = 0; i < 3; i++)
      {
        data_on += data[i];
      }
      if (data_on == "+CGSN")
      {
        data_tempt = data_on;
        sta++;
      }
    }
  }
  data = "";
  data_on = "";
}

void BC20_reset()
{ // reset BC20
  mySerial.println("AT+QRST=1");
  while (sta == 1)
  { // 等待模組訊息回覆
    serial_read();
    delay(1 * 1000);
    count++;
    if (count > 10)
    { // 超過10秒未有回覆，重新啟動系統
      count = 0;
      Serial.println(noreply);
      resetFunc();
    }
  }
  count = 0;
}

void set_band(int band)
{ // 設置頻段
  String AT_band = "AT+QBAND=1,";
  AT_band.concat(band);
  mySerial.println(AT_band);
  while (sta == 2)
  { // 等待模組訊息回覆
    serial_read();
    delay(1 * 1000);
    count++;
    if (count > 10)
    { // 超過10秒未有回覆，重新啟動系統
      count = 0;
      Serial.println(noreply);
      resetFunc();
    }
  }
  count = 0;
}

void ask_for_IP()
{ // 查詢IP狀況
  mySerial.println("AT+CGPADDR=1");
  while (sta == 3)
  { // 等待模組訊息回覆
    serial_read();
    delay(1 * 1000);
    count++;
    if (count > 10)
    { // 每10秒問一次IP狀況
      IP_count1++;
      mySerial.println("AT+CGPADDR=1");
      count = 0;
      if (IP_count1 > 1)
      { // 一分鐘後仍沒找到IP，重新開機
        Serial.println(noreply);
        // resetFunc();
        break;
      }
    }
  }
  count = 0;
  IP_count1 = 0;
}

void build_MQTT_connect(String IP, String port)
{ // 建立TCP連線通道
  data_tempt = "AT+QMTOPEN=0,";
  data_tempt.concat(IP);
  data_tempt.concat(",");
  data_tempt.concat(port);
  mySerial.println(data_tempt);
}

void connect_MQTT()
{ // 建立TCP連線通道
  data_tempt = "AT+QMTCONN=0,";
  data_tempt.concat(clientID);
  data_tempt.concat(",");
  data_tempt.concat(user);
  data_tempt.concat(",");
  data_tempt.concat(pass);
  mySerial.println(data_tempt);
}

void reading(int sta_pre, int sta)
{
  while (sta_pre == sta)
  { // 等待模組訊息回覆
    serial_read();
    delay(1 * 1000);
    break;
    count++;
  }
  if (count > 10)
  { // 超過10秒未有回覆，
    count = 0;
    Serial.println("no replay, send next data after 2 seconds");
    delay(1 * 1000);
    sta_pre++;
  }
  count = 0;
  sta_pre = sta;
}

void Publish_MQTT(String topic, String message)
{ // 資料上傳function
  Serial.println("Sending data......");
  int sta_pre = sta;
  // AT指令：AT+QMTPUB=0,0,0,0,"topic","message"
  // topic: TeamXX/NBIoT/Temp TeamXX/NBIoT/Humi TeamXX/NBIoT/Sensors
  // message: tempture huminity {"Temperature":tempture,"Humidity":huminity}
  data_tempt = "AT+QMTPUB=0,0,0,0,";
  data_tempt.concat(topic);
  data_tempt.concat(",");
  data_tempt.concat(message);
  Serial.println(data_tempt);
  mySerial.println(data_tempt);
  delay(1000);
  reading(sta_pre, sta);
}

void BC20_initail()
{
  BC20_reset();
  delay(1 * 1000);
  set_band(band);
  delay(20 * 1000); // 等待20秒連線
  ask_for_IP();     // 查詢IP狀況
  delay(5 * 1000);
}

void long_delay(int hr, int minut, int sec)
{ // 設定delay時間長度
  for (int i = 0; i < (hr * 60 * 60 + minut * 60 + sec); i++)
  {
    delay(1000);
  }
}
