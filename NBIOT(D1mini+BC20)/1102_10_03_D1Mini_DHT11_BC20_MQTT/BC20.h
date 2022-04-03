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
int data_len = 0;

String server_IP = "140.128.99.71";
String server_port = "1883";
String clientID = "\"D1mini-qweasdzxc312"; // 請修改成唯一值，可用亂數
String user = "\"thulora\"";
String pass = "\"hpclab99\"";
String noreply = "no reply, reset all";
String MQTTtopic;
String MQTTmessage;

String MQTTGetScript = "AT+QMTSUB=0,1,\"Forensics/PC\",0";
String MQTTGetmessage = "";
String GetSync()
{
  Serial.println("=======GetTopic=========");
  bool get_msg = true;
  data = "";

  mySerial.println(MQTTGetScript);
  int check = 1;
  int wait = 0;
  while (get_msg)
  {
    delay(1000);
    if (wait < 120)
    {
      while (mySerial.available())
      {                                                // Check if there is an available byte to read,
        delay(10);                                     // Delay added to make thing stable
        char c = mySerial.read();                      // Conduct a serial read
        if (data == "+QMTRECV: 0,0,\"Forensics/PC\",") // Check Flag
        {
          // Serial.println("+QMTRECV: 0,0,\"Forensics/Sensor\",");
          check = 2;
          data = "";
        }
        if (c == '\n')
        {
          if (check == 2)
          {
            // Serial.println("1 Get DATA = " + data);
            delay(1000);
            data_len = data.length();
            // Serial.println("3 Get data_len = ");
            Serial.println(data.length());
            data_len = data_len - 2;

            data = data.substring(1, data_len);
            Serial.println("Get DATA = " + data);
            delay(1000);

            get_msg = false;
            MQTTGetmessage = data;
            return MQTTGetmessage;
            Serial.println("=======GetTopic=========");
            break;
          }
          else
          {
            // Serial.println("data = " + data);
            if (data == "ERROR")
            {
              Serial.println("data = ERROR");
              get_msg = false;
              resetFunc();
            }
            else
            {
              // Serial.println("data != ERROR");
            }
            data = "";
          }
        }
        else
        {
          data += c;
        }
      }
      wait++;
    }
    else
    {
      Serial.println("Out of limit time Fetching ");
      get_msg = false;
      Serial.println("=======GetTopic=========");
    }
  }
}

void CleanBuffer()
{
  while (mySerial.available())
  {                           // Check if there is an available byte to read,
    delay(10);                // Delay added to make thing stable
    char c = mySerial.read(); // Conduct a serial read
    data += c;
  }
  // Serial.println("CleanBuffer = " + data);
  data = "";
}

String BC20_CIMI()
{ // Get Sim 卡號
  // Serial.println("BC20_CIMI: AT+CIMI");
  mySerial.println("AT+CIMI");
  sta = 5;
  int check = 1;
  String tmp_data = "";
  while (sta == 5)
  { // 等待模組訊息回覆
    while (mySerial.available())
    {                           // Check if there is an available byte to read,
      delay(10);                // Delay added to make thing stable
      char c = mySerial.read(); // Conduct a serial read

      if (data == "AT+CIMI") // Check Flag
      {
        // Serial.println("Get AT+CIMI");
        check = 2;
      }

      if (c == '\n')
      {
        if (check == 2)
        {
          check = 3;
        }
        else if (check == 3)
        {
          // Serial.println("Get SIMID = " + data);
          break;
        }
        // Serial.println("data = " + data);
        data = "";
      }
      else
      {
        data += c;
      }
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
      resetFunc();
    }
  }
  count = 0;
  delay(5 * 1000);
  return data;
}

String BC20_CGSN()
{ // Get IMEI 卡號
  // Serial.println("BC20_CGSN: AT+CGSN");
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
      if (data == "+CGSN: ")
      {
        // Serial.println("Get +CGSN: ");
        data = "";
        check = 2;
      }

      if (c == '\n')
      {
        if (check == 2)
        {
          // Serial.println("Get IMEI = " + data);
          break;
        }
        data = "";
      }
      else
      {
        data += c;
      }
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
  // Serial.println(data_tempt);
  delay(1000);
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
