#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <SoftwareSerial.h>

SoftwareSerial S76SXB(14, 12); // RX=D5, TX=D6 to LoRa:EK-S76SXB
String lora_string;
char readcharbuffer[20];
int readbuffersize;
char temp_input;
String second_response;

#define WIFI_SSID "ISLab"          // WIFI SSID here
#define WIFI_PASSWORD "chlin33830" // WIFI password here

IPAddress server_addr(192, 168, 1, 200); // change to you server ip, note its form split by "," not "."
int MYSQLPort = 3306;                    //mysql port default is 3306
char user[] = "localuser";               // Your MySQL user login username(default is root),and note to change MYSQL user root can access from local to internet(%)
char pass[] = "qweasdzxc";               // Your MYSQL password
char ddb[] = "lora_local";
String MSG = "";
bool qurysearch_ok = false;
bool quryinsert_ok = false;
int nRow = 10;
int nCol = 3;
String quryArray[10][3];
String ACK = "2244";

WiFiClient client;
MySQL_Connection conn((Client *)&client);

int status = 0;

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
    //Serial.println("Connecting...");
    delay(200);
    if (conn.connect(server_addr, 3306, user, pass))
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

  //Creat MySQL Connect Item
  cur_mem->execute(QUERY_SQL);
  //get columns
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

  //Creat MySQL Connect Item
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
    if (conn.connect(server_addr, 3306, user, pass))
    {
      delay(500);
      Serial.println("Successful reconnect!");
      //Insert into DB
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

bool lorasend(String MSG)
{
}

void setup()
{
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  //------------------------------
  S76SXB.begin(9600); // LoRaWAN EK-S76SXB Start
  // for rest EK-S76SXB
  pinMode(2, OUTPUT);   // EK-S76SXB NRST(PIN5) to LinkIt 7697 P12
  digitalWrite(2, LOW); //
  delay(0.1 * 1000);    // wait for 0.1 second
  pinMode(2, INPUT);    // for rest EK-S76SXB
  delay(2 * 1000);

  S76SXB.print("mac join abp"); // Join LoRaWAN
  Serial.println("mac join abp");
  delay(5 * 1000);
  //------------------------------
  Serial.println("Communication Started \\n\\n");
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //try to connect with wifi
  Serial.print("Connecting to ");
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
  Serial.println(WiFi.localIP()); //print local IP address

  // connecting to the MySQL server
  Serial.println("DB - Connecting...");
  //try to connect to mysql server
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
/*
String three_way_validate(String val_check)
{
  global status
    //status: 0(尚未收到新的ACK+SYN))/1(收到ACK+SYN，送出ACK)/2(收到ACK，確認送出ACK)
    if (status == 0)
    { //First shake
        //Send SYN
        //First return = SYN+ACK
        Serial.println("Have not recieved");
        return val_check;
    }
    else if (status == 1) //Second shake
    {
        //Send ACK
        //First return = SYN+ACK
        status = int(val_check[1]);
        Serial.println("Third shake complete send");
        return val_check;
    }
    else
    {
        Serial.println("===System caught exception===");
        Serial.println("three_way_status = ", status);
    }
}
*/

// the loop function runs over and over again forever
void loop()
{
  //status:
  //0(尚未收到Data)
  //1(送出SYN,尚未收到新的ACK+SYN)
  //2(收到ACK+SYN，送出ACK，尚未收到ACK)
  //3(收到ACK，確認送出ACK，上傳Data中)

  String lora_string = "mac tx ucnf 2 "; // for tx header
  //New Update Check Module
  if (qurysearch())
  {
    Serial.println("已完成Data擷取, 進入status 1");

    //1(送出SYN,尚未收到新的ACK+SYN)
    status = 1;
    Serial.println("Success Qury");
    digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(1000);                     // wait for a second
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
    MSG = quryArray[0][2];
    Serial.print("MSG length = ");
    Serial.println(MSG.length());
    //lora_string.concat(MSG);
    Serial.println(lora_string + MSG);
    //Success send and go to update status
    lora_string.concat(MSG);
    Serial.println("GET MSG");
    Serial.println(lora_string);
    bool status_1 = true;
    bool status_2 = true;
    bool status_3 = true;
    String msg_syn = "";
    String msg_ack_syn = "";
    String msg_ack = "";
    String msg_ack_flag = "1a";
    String msg_ack_validate = "";
    String msg_syn_validate = "";
    int ack_start = 0;
    int ack_end = 0;
    int len_response = 0;
    while (status_1)
    {
      //Send SYN
      msg_syn = MSG;
      Serial.println(lora_string);
      Serial.println("LoRa sending...");
      delay(0.01 * 1000);
      S76SXB.print(lora_string);

      Serial.println("");
      delay(9 * 1000);

      // Get ACK+SYN
      if (S76SXB.available())
      {
        second_response = S76SXB.readString();
        Serial.println(second_response);
        char charBuf[300];
        second_response.toCharArray(charBuf, 300);
        //Check rx
        if (second_response.indexOf("rx") > 0)
        {
          Serial.println("Yes rx!");
          //Check rx within ack and get it
          if (second_response.indexOf(msg_ack_flag) > 0)
          {
            // Get ACK+SYN
            ack_start = second_response.indexOf(msg_ack_flag);

            Serial.println("===================");
            String msg_ack_syn_tmp = "";
            for (int i = ack_start; i <= sizeof(charBuf); i++)
            {
              if (charBuf[i] == '\r')
              {
                Serial.println("detext \r");
                break;
              }
              //Serial.print("loop");
              Serial.print(charBuf[i]);
              msg_ack_syn_tmp = msg_ack_syn_tmp + charBuf[i];
            }
            msg_ack_syn = msg_ack_syn_tmp;
            Serial.print("Get ack_syn => ");
            Serial.print(msg_ack_syn);

            // Get ACK
            //取得ACK結尾
            msg_ack_syn_tmp = "";
            ack_start = 0;
            ack_end = msg_ack_syn.indexOf(msg_syn) - 1;
            char charBuf_ack[300];
            //將msg_ack_syn轉換為charbuffer
            msg_ack_syn.toCharArray(charBuf_ack, 300);
            for (int i = ack_start; i < ack_end; i++)
            {
              //Serial.print("loop");
              Serial.print(charBuf_ack[i]);
              msg_ack_syn_tmp = msg_ack_syn_tmp + charBuf_ack[i];
            }
            msg_ack = msg_ack_syn_tmp;

            // 對比 SYN
            //取得ACK結尾

            msg_ack_syn_tmp = "";
            ack_start = msg_ack_syn.indexOf(msg_syn);
            char charBuf[300];
            msg_ack_syn.toCharArray(charBuf, 300);
            for (int i = ack_start; i < sizeof(charBuf); i++)
            {
              if (charBuf[i] == ' ')
              {
                Serial.println("detext blank");
                break;
              }
              //Serial.print("loop");
              Serial.print(charBuf[i]);
              msg_ack_syn_tmp = msg_ack_syn_tmp + charBuf[i];
            }
            msg_syn_validate = msg_ack_syn_tmp;
            Serial.print("Get syn_validate => ");
            Serial.print(msg_syn_validate);

            if (msg_syn == msg_syn_validate)
            {
              //已完成SYN對比與ACK擷取, 進入status 2
              status_1 = false;
              Serial.print("SYN = ");
              Serial.println(msg_syn);
              Serial.print("ACK+SYN = ");
              Serial.println(msg_ack_syn);
              Serial.print("ACK = ");
              Serial.println(msg_ack);
              Serial.println("已完成SYN對比與ACK擷取, 進入status 2");
              Serial.println("===================");
            }
            else
            {
              Serial.println("SYN對比失敗, 再次驗證");
              Serial.println("===================");
            }
          }
          else
          {
            Serial.println("Get unknown data!!");
          }
        }
        else
        {
          Serial.println("NO rx!");
        }
      }
      Serial.println("=== End of LoRaWAN one round ===");
      delay(46 * 1000); // 14sec + 46sec = 60sec = 1 min
    }
    while (status_2)
    {
      lora_string = "mac tx ucnf 2 12";
      lora_string.concat(msg_ack);
      Serial.println(lora_string);
      Serial.println("LoRa sending...");
      delay(0.01 * 1000);
      S76SXB.print(lora_string);
      char charBuf[300];
      Serial.println("");
      delay(9 * 1000);

      // Get ACK
      if (S76SXB.available())
      {
        second_response = S76SXB.readString();
        Serial.println(second_response);

        second_response.toCharArray(charBuf, 300);
        //Check rx
        if (second_response.indexOf("rx") > 0)
        {
          Serial.println("Yes rx!");
          //Check rx within ack and get it
          if (second_response.indexOf(msg_ack) > 0)
          {
            // Get ACK+SYN
            ack_start = second_response.indexOf(msg_ack);

            Serial.println("===================");
            String msg_ack_syn_tmp = "";
            for (int i = ack_start; i <= sizeof(charBuf); i++)
            {
              if (charBuf[i] == '\r')
              {
                Serial.println("detext \r");
                break;
              }
              //Serial.print("loop");
              Serial.print(charBuf[i]);
              msg_ack_syn_tmp = msg_ack_syn_tmp + charBuf[i];
            }
            msg_ack_validate = msg_ack_syn_tmp;
            Serial.print("Get ack_validate => ");
            Serial.print(msg_ack_validate);

            // 對比 ACK
            //取得ACK結尾
            if (msg_ack == msg_ack_validate)
            {
              //已完成ACK對比, 進入status 3
              status_2 = false;
              Serial.println("已完成ACK對比, 進入status 3");
              Serial.println("===================");
            }
            else
            {
              Serial.println("ACK對比失敗, 再次驗證");
              Serial.println("===================");
            }
          }
          else
          {
            Serial.println("Get unknown data!!");
          }
        }
        else
        {
          Serial.println("NO rx!");
        }
      }
      Serial.println("=== End of LoRaWAN one round ===");
      delay(46 * 1000); // 14sec + 46sec = 60sec = 1 min
    }
    while (status_3)
    {
      if (lorasend(lora_string))
      {
        Serial.println("success");
        //quryupdate(quryArray[0][0]);
        break;
      }
      else
      {
        Serial.println("failed");
      }
      delay(30 * 1000);
    }

    Serial.println("====================");
  }
  else
  {
    //0(尚未收到Data)
    status = 0;
  }
  delay(15000);
}