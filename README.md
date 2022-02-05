# Lora_Transmission
---
title: LORA+Arduino
tags: LORA
description: LORA Send
---
# 遠傳輸協定採證專案
## 架構:
> 目前在做的是架設Local Server DB 
在Local端改以Local DB做資料傳輸
RPI > 傳送圖片>Local DB
Local DB>將結果塞入圖片的結果欄位
RPI > 讀取DB是否有結果 > 是的話> 將結果與GPS資料與時間溫溼度Merged > 將Data Encrypted > 將要傳送之 Data 傳入 DB
NodeMCU > 讀取DB > 將其利用Lora傳送至MQTT
以下是Activity Diagram UML
---
1. 將擷取之資料組合成特定格式(json)於樹梅派端，這端將組合所有Sensor資訊(GPS、溫度、濕度)
2. 將此格式透過以下方式傳入傳輸模組(LORA)
3. NodeMCU 將所接取之資料透過LORA模組進行上傳遠端 MQTT 基站並且回控
4. 由Server端利用Python script擷取MQTT之訊息並且Insert目標DB

> 以下是架構圖:
![](https://i.imgur.com/Zb3zQFy.png)


## Raspiberry pi 4
### 負責功能有以下:
#### 1. Check Module:
>     確認有新的已辨識資料可傳送
#### 2. Data collect Module:
>     蒐集目前的GPS與溫度濕度時間與辨識資料整合
#### 3. Crypted Module
>     將要傳送之資料經過加密模組加密


```python=
import serial
import time
import string
import mysql.connector
from mysql.connector import Error
keyword = "LORA"

#Check Module check new Data
def searchDB():
    #DB Get config
	dhost = "192.168.31.200" #serverip
	ddb = "lora_local"
	duser = "localuser"
	dpass = "qweasdzxc"
	try:
            #DB connection
            conn = mysql.connector.connect(
                host = dhost,
                database = ddb,
                user = duser,
                password = dpass
            )
            #DB Get SQL
            sql = "select * from predict where PCcheck='1' and checkType != '' and RPIcheck=''"
            cursor = conn.cursor()
            cursor.execute(sql)
            result = cursor.fetchall()
            cursor.close()
            conn.close()
            if(result==[]):
                return 0
            else:
                return result
    except Error as e:
        print("DB connect Error")

#Data collect Module
def data_get(obj):
  #########GET ALL DATA about Temp+Humi+GPS FROM Sensor#########

  #########GET GPS#########
  port="/dev/ttyAMA0"
  ser=serial.Serial(port, baudrate=9600, timeout=0.5)
  except_counter = 0
  sender = "26008625"
  while 1:
    try:
      newdata=ser.readline()
      if (str(newdata[0:6]) == "b'$GPRMC'"):
        print("===========================")
        latoriginal = str(newdata).split(",")
        tim=latoriginal[1].split(".")[0]
        lat="0"+latoriginal[3].split(".")[0]+latoriginal[3].split(".")[1]
        lng=latoriginal[5].split(".")[0]+latoriginal[5].split(".")[1]
        gps = "Latitude=" + str(lat) + " and Longitude=" + str(lng)
        lorasender = obj+sender+tim+lat+lng
        print(lorasender)
        print("===========================")
        except_counter = 0
        return lorasender
        break
      
    except serial.serialutil.SerialException:
      except_counter +=1
      print(except_counter)
      return False
  #########GET GPS#########

#Data collect Module
def sendDB(objid,cipher_text,merged):
  #DB Send config
  dhost = "192.168.31.200" #serverip
  ddb = "lora_local"
  duser = "localuser"
  dpass = "qweasdzxc"
  print("sendDB")
  try:
    conn = mysql.connector.connect(
      host = dhost,
      database = ddb,
      user = duser,
      password = dpass
    )
    
    print(objid)
    print(merged)
    #DB INSERT SQL
    sql = "INSERT INTO send (id,send,send_crypted) VALUES ("+str(objid)+",'"+str(merged)+"','"+str(cipher_text)+"')"
    print(sql)
    cursor = conn.cursor()
    cursor.execute(sql)
    conn.commit()
    print("insert success")
    #DB update SQL
    sql = "update predict set RPIcheck='1' where id= "+str(objid)
    print(sql)
    cursor = conn.cursor()
    cursor.execute(sql)
    conn.commit()
    print("update success")
    
    cursor.close()
    conn.close()
    if(result==[]):
      return 0
    else:
      return result
  except Error as e:
    print("DB connect Error")

#Crypted Module
def txt_shift(txt, shift):
  result = ""
  hexset=["0","1","2","3","4","5","6","7","8","9","a","b","c","d","e","f"]
  locate = ""
  for idi in range(0, len(txt)):
      for idj in range(0, len(hexset)):
          if(str(txt[idi]) == hexset[idj]):
              locate = idj
              break
      locate = locate+shift
      locate = locate%16
      result = result + str(hexset[locate])
  return result
#Crypted Module
def caesar_encryption(txt, shift):
  return txt_shift(txt, shift)
#Crypted Module
def caesar_decryption(txt, shift):
  return txt_shift(txt, -1 * shift)

shift_amount = 8

def checklength(objid):
  a = int(objid)
  b = len(str(a))
  if(b%2)==0:
    return objid
  else:
    objid = str(0)+str(objid)
    return objid
		
print("initial")
while True:
  #########Get Server Data#########
  #get new Identify data from Server module
  result = searchDB()
  time.sleep(1)
  if(result):
    objid = result[0][0]
    obj = result[0][2]
    #get data from gps module
    original = data_get(obj)
    #check length is even
    originalid = checklength(objid)
    merged = originalid+original
    if(merged):
      print(f"原始明文: {merged}")
      cipher_text = caesar_encryption(merged, shift_amount)
      print(f"加密密文: {cipher_text}")
      decryption_cipher_txt = caesar_decryption(cipher_text, shift_amount)
      print(f"解密結果: {decryption_cipher_txt}")
      sendDB(objid,cipher_text,merged)
    else:
      print("GPS merged failed")
  else:
    print("no result")
		
	#########Get Server Data#########

```


## NodeMCU Module 
### 負責功能有以下:
#### 1. Lora Sending Engine:
> 1. New Update Check Module
    >確認有新的資料可傳送
> 2. Lora Sender Module
    > 將資料透過Lora傳送
#### 2. Lora Check Engine
> 1. Lora Reciever Module
    >用以接收Three-Way hand shake回控(tx)
> 2. Three-Way hand shake
    > 確認資料符合訂定之回控
    > SYN (Synchronize sequence numbers)如果有設置，才發出連線請求，用來同步序列號。
    > ACK (Acknowledgment field significant)如果有設置，使確認號欄位有效。
    > SYN+ACK(用以中間同步確認Server端有收到)
    > 舉例為以下:
    > SYN: Crypted_Data
    > SYN+ACK: Crypted_Data+checknum
    > ACK: checknum
    > checknum: 103
    
 ![](https://s3.notfalse.net/wp-content/uploads/2016/12/24160105/Three-way-Handshake-ex2.png)
        


> 這邊部分是實際線路圖
![](https://i.imgur.com/v7i98uz.png)

```cpp=
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

                        if(msg_syn == msg_syn_validate){
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
                        }else{
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
                        if(msg_ack == msg_ack_validate){
                            //已完成ACK對比, 進入status 3
                            status_2 = false;
                            Serial.println("已完成ACK對比, 進入status 3");
                            Serial.println("===================");
                        }else{
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
```


## Python MQTT crawler Module 
### 負責功能有以下:
#### 1. Lora MQTT Crawler:
> 1. New Update Check Module
    >確認有新的資料
#### 2. Lora Check Engine
> 1. Lora Reciever Module
    >用以接收Three-Way hand shake回控(tx)
> 2. Three-Way hand shake
    > 確認資料符合訂定之回控
    > SYN (Synchronize sequence numbers)如果有設置，才發出連線請求，用來同步序列號。
    > ACK (Acknowledgment field significant)如果有設置，使確認號欄位有效。
    > SYN+ACK(用以中間同步確認Server端有收到)
    > 舉例為以下:
    > SYN: Crypted_Data
    > SYN+ACK: Crypted_Data+checknum
    > ACK: checknum
    > checknum: 103

#### 3.Data Insert Module
> Insert into DB


```python=
import pymysql
import logging
import json
import paho.mqtt.client as mqtt
import datetime as datetimeLibrary
import time
import mysql.connector
from mysql.connector import Error
import datetime




######### DB module  #########
def connectDb(dbName):
    try:
        mysqldb = pymysql.connect(
                host="140.128.101.127",
                user="lora",
                passwd="lora",
                database=dbName)
        print('Connected')
        return mysqldb
    except Exception as e:
        logging.error('Fail to connection mysql {}'.format(str(e)))
    return None


conn = connectDb('lora_server')
if conn is not None:
    cur = conn.cursor()    
def insertDB(checkType,time,temp,humidity,gps):
    sql = "INSERT INTO result (type, time, temperature, humidity, gps) VALUES (%s, %s, %s, %s, %s)"
    val = (checkType, time,temp ,humidity, gps)
    cur.execute(sql, val)
    conn.commit()
    print(cur.rowcount, "record inserted.")
    print(sql, val)

def selectDB():
    sql = 'select * from result'
    if cur is not None:
        cur.execute(sql)
        resultall = cur.fetchall()
        #print(resultall)
        print ('Fetch all rows:')
        for row in resultall:
            print (row) #數字代表column
######### DB module  #########

######### crypt module  #########
def txt_shift(txt, shift):
    result = ""
    hexset=["0","1","2","3","4","5","6","7","8","9","a","b","c","d","e","f"]
    locate = ""
    for idi in range(0, len(txt)):
        for idj in range(0, len(hexset)):
            if(str(txt[idi]) == hexset[idj]):
                locate = idj
                break
        locate = locate+shift
        locate = locate%16
        result = result + str(hexset[locate])
    return result

def caesar_encryption(txt, shift):
    return txt_shift(txt, shift)

def caesar_decryption(txt, shift):
    return txt_shift(txt, -1 * shift)
######### crypt module  #########

######### Three-way Handshake module  #########
def three_way_status(receive_data=""):
    #status = 0(尚未收到SYN) / 1(收到SYN，送出SYN+ACK) / 2(收到ACK回傳，確認訊息已收到，準備上傳DB)
    global status
    global ACK_SYN_data
    global ACK
    if(receive_data==""):
        print("no receive_data")
    if(status==0):
        SYN = receive_data
        ACK_SYN_data = ACK + SYN
        #(收到SYN，送出SYN+ACK)
        status = 1
        print("收到SYN，送出SYN+ACK")
        return ACK_SYN_data
    elif(status==1):
        ACK_received = receive_data
        ACK_length = len(ACK)
        if(ACK==ACK_received[0:ACK_length]):
            status = 2
            #收到ACK回傳，確認訊息已收到，準備上傳DB
            print("收到ACK回傳，確認訊息已收到，正在準備上傳DB")
            return True
        else:
            SYN = receive_data
            ACK_SYN_data = ACK + SYN
            #(收到SYN，送出SYN+ACK)
            print("仍收到SYN，繼續送出SYN+ACK")
            return False
    elif(status==2):
        print("收到ACK回傳，確認訊息已收到，正在準備上傳DB")
        return True
    
    else:
        print("error: "+status)
        print("receive_data: "+receive_data)

######### Three-way Handshake module  #########
######### RX send module  #########
def RX_sender(gwid,macAddr,ACK_SYN_data):
    #payload = '[{"macAddr":"'+macAddr+'","data":"'+ACK_SYN_data+'","gwid":"'+gwid+'","extra":{"port":2,"txpara":"2"}}]'
    payload = [{'macAddr':macAddr,'data':ACK_SYN_data,'gwid':gwid,'extra':{'port':2,'txpara':'2'}}]
    topic = "GIOT-GW/DL/"+gwid
    print (json.dumps(payload))
    print("topic = "+topic)
    try:
        client.publish( topic, json.dumps(payload))
        return True
    except:
        print("RX_sender publish error")
        return False

######### RX send module  #########



######### MQTT Crawler module  #########

# 當地端程式連線伺服器得到回應時，要做的動作
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # 將訂閱主題寫在on_connet中
    # 如果我們失去連線或重新連線時
    # 地端程式將會重新訂閱
    client.subscribe("GIOT-GW/UL/1C497B455949")
    client.subscribe("GIOT-GW/UL/1C497B455A27")
    client.subscribe("GIOT-GW/UL/1C497BFA3280")
    client.subscribe("GIOT-GW/UL/80029C5A3306")
    client.subscribe("GIOT-GW/UL/80029C5A33A5")

# 當接收到從伺服器發送的訊息時要進行的動作
def on_message(client, userdata, msg):
    global status
    global ACK_SYN_data
    global ACK
	# print(msg.topic+" "+msg.payload.decode('utf-8'))   # 轉換編碼
    #utf-8才看得懂中文
    #print(msg.topic+" "+str(msg.payload))
    #print(msg.payload)
    #print(len(str(msg.payload)))
    if(len(str(msg.payload))<6):
        print("initial")
    else:
        #json module
        d = json.loads(msg.payload)[0]
        gwid = d['gwid']
        macAddr = d['macAddr']
        receive_data = d['data']
        data_length = len(receive_data)


        if(macAddr =="0000000051000002" or macAddr =="0000000051000000" or macAddr =="0000000051000001" or macAddr =="0000000051000003"):
            if(receive_data==""):
                print("===No data no rx===")
            else:
                print("===============================================================")
                # 取得我們對應之卡號，印出基本訊息
                print(msg.topic)
                now = datetime.datetime.now()
                print ("Current date and time : "+now.strftime("%Y-%m-%d %H:%M:%S"))
                print("receive_data = "+receive_data)
                print("gwid="+gwid+",macAddr="+macAddr+",receive_data="+receive_data)
                print(d)
                
                
                print("===start rx===")	  
                payload = [{'macAddr':'0000000051000002','data':'22','gwid':'000080029c55a773','extra':{'port':2,'txpara':'2'}}]
                print (json.dumps(payload))
                ##要發布的主題和內容
                client.publish( "GIOT-GW/DL/00001c497b431d31", json.dumps(payload))
                print("===End rx===")	  
                
                if(status==0):
                    ACK_SYN_data = three_way_status(receive_data)
                    if(status==1):
                        RX_sender(gwid,macAddr,ACK_SYN_data)
                        print("status = 1，send RX")

                    else:
                        print("error in status = 0")
                elif(status==1):
                    if(three_way_status(receive_data)):
                        if(status ==2):
                            print("Insert into DB")
                    else:
                        ACK_SYN_data = receive_data
                        print(RX_sender(gwid,macAddr,ACK_SYN_data))
                        print("error in status = 1，resend RX")
                        
                        #rx_data = ACK+ACK+receive_data
                        #print("===start rx===")	  
                        #payload = [{'macAddr':'0000000051000002','data':'11','gwid':'00001c497bf88385','extra':{'port':2,'txpara':'2'}}]
                        #print (json.dumps(payload))
                        ###要發布的主題和內容
                        #client.publish( "GIOT-GW/DL/00001c497b431d31", json.dumps(payload))
                        #payload = [{'macAddr':'0000000051000002','data':'22','gwid':'000080029c55a773','extra':{'port':2,'txpara':'2'}}]
                        #print (json.dumps(payload))
                        
                elif(status==2):
                    print("Insert into DB")


            #client.publish( "GIOT-GW/DL/000080029c55a773", json.dumps(payload))
            #client.publish( "GIOT-GW/DL/00001c497b431d31", json.dumps(payload))
            # time.sleep(10)

# 連線設定
client = mqtt.Client()            # 初始化地端程式 
client.on_connect = on_connect    # 設定連線的動作
client.on_message = on_message    # 設定接收訊息的動作

client.username_pw_set("course","iot999")      # 設定登入帳號密碼 
client.connect("140.128.99.71", 1883, 60) # 設定連線資訊(IP, Port, 連線時間)

# 開始連線，執行設定的動作和處理重新連線問題
# 也可以手動使用其他loop函式來進行連接
client.loop_forever()

######### MQTT Crawler module  #########
```
