#include <Arduino.h>
#include <SoftwareSerial.h>
#include "BC26.h"
#include "ideaschain.h"


const String noreply = "no reply, reset all";

SoftwareSerial mySerial(8, 9);
int time_count = 0;
int reset_count = 0;               //  系統重新啟動計時器宣告


String data = "";                  //  儲存來自BC26回覆的字串參數
int sta = 1;                       //  迴圈的status參數判斷
int count = 0;                     //  副程式中來自BC26的回覆次數計數器
int band = 8;                      //  設定頻段(B8)
int IP_count1 = 0;

void(* resetFunc) (void) = 0;      //  宣告系統重置參數


void errorLog(String message) {
  Serial.println(message);
}

void debugLog(String message) {
#if DEBUG_LEVEL >= 1
  Serial.println(message);
#endif
}

void infoLog(String message) {
#if DEBUG_LEVEL >= 2
  Serial.println(message);
#endif
}

void serial_read(){
  int retries = 0;
  while (retries<100){  
    //Check if there is an available byte to read
    if(!mySerial.available()){
      retries++;
      continue;
    } 
    retries = 0;
    delay(10); //Delay added to make thing stable 
    char c = mySerial.read(); //Conduct a serial read
    if (c == '\n') {
      infoLog("got LF");
      //Exit the loop when the '\n' is detected after the word
      break;
    } 
    data += c; //Shorthand for data = data + c
  }

  data.trim();
  if (data.length() > 0) { 
    // data內有值 (AT command response)
    //delay(100);
    debugLog("data read:"+data);
    if(sta == 1){
      count++;
    }

    if(sta == 1){
      if(count >= 1){         //  Turn on BC26 status
        sta++;
        count = 0;
      }
    }else if(sta == 2){       //  wait for IP status
      if(data.startsWith("+IP") || data.startsWith("+CGPADDR: 1")){
        sta=3;
      }
    }else if(sta == 3){       //  與MQTT伺服器連線
      if(data == "OK"){
        delay(3*1000);
        sta++;
      } else if(data.startsWith("+QMTOPEN:")){
        sta++;
      } else if(data.startsWith("+CME ERROR:")) {
        // open for MQTT failed
      }
    }else if(sta == 4){       //  與裝置建立通道並連線
      if(data == "OK"){
        delay(5*1000);
        sta++;
      } else if(data.startsWith("+QMTCONN")) {
        sta++;
      }
    }else if(sta > 4){        //  傳輸資料與伺服器端訊息回傳
      if(data == "OK"){
        if(sta == 5)
           delay(1000);
        sta++;
      }else if(data.indexOf("+QMTCLOSE:")>=0){
        sta = 3;
      }else if(data.indexOf("+QMTPUB")>=0){
        sta = 6;
      }
    }
    infoLog("sta is " + String(sta));
    data = "";
  }
}

void BC26_reset(){                                //  reset BC26
  mySerial.println("AT+QRST=1");
  while (sta == 1) {          //  等待模組訊息回覆
    serial_read();
    delay(1*1000);
    count++;
    if (count > 10) {        //  超過10秒未有回覆，重新啟動系統
      count = 0;
      errorLog(noreply);
      resetFunc();
    }
  }
  count = 0;
}

void ask_for_IP(){                                //  查詢IP狀況
  mySerial.println("AT+CGPADDR=1");
  while (sta == 2) {          //  等待模組訊息回覆
    serial_read();
    delay(1*1000);
    count++;
    if (count > 10) {       //  每10秒問一次IP狀況
      IP_count1++;
      mySerial.println("AT+CGPADDR=1");
      count = 0;
      if(IP_count1 > 6){      //  一分鐘後仍沒找到IP，重新開機
        errorLog(noreply);
        resetFunc();
      }
    }
  }
  count = 0;
  IP_count1 = 0;
}

void build_MQTT_connect(){                         //  建立TCP連線通道
  debugLog("build_MQTT_connect");
  String atCmd = "AT+QMTOPEN=0,\"";
  atCmd.concat(server_IP);
  atCmd.concat("\",");
  atCmd.concat(server_port);
  mySerial.println(atCmd);

  while (sta == 3) {          //  等待模組訊息回覆
    serial_read();
    delay(1*1000);
    count++;
    if (count > 10) {        //  超過10秒未有回覆，重新啟動系統
      count = 0;
      errorLog(noreply);
      resetFunc();
    }
  }

  count = 0;
}

void connect_MQTT(){                          //  與伺服器連接
  debugLog("connect_MQTT");
  String atCmd = "AT+QMTCONN=0,\"";
  atCmd.concat(clientId);
  atCmd.concat("\",\"");
  atCmd.concat(username);
  atCmd.concat("\"");
  mySerial.println(atCmd);
  serial_read();

  while (sta == 4) {          //  等待模組訊息回覆
    serial_read();
    delay(1*1000);
    count++;
    if (count > 10) {        //  超過10秒未有回覆，重新啟動系統

      count = 0;
      errorLog(noreply);
      resetFunc();
    }
  }
  count = 0;
}

void close_MQTT(){                          //  與伺服器斷開連線
  debugLog("close_MQTT");
  String atCmd = "AT+QMTCLOSE=0";
  mySerial.println(atCmd);
//  delay(1*1000);
  int retries=0;
  while (sta >= 5 && retries < 5) { 
    serial_read();
    delay(1*1000);
    retries++;
  }
  sta = 3;
}

void publish_MQTT(String topic, String message){
  debugLog("publish_MQTT");
  String atCmd = "AT+QMTPUB=0,0,0,0,";
  atCmd.concat(topic);
  atCmd.concat(",");
  atCmd.concat(message);

  debugLog(atCmd);

  mySerial.println(atCmd);
  delay(3*1000);
  while (sta == 5) {          //  等待模組訊息回覆
    serial_read();
    delay(1*1000);
    count++;
    if (count > 5) {        //  超過5秒未有回覆，繼續行程....
      count = 0;
      break;
    }
  }
  delay(1*1000);
}

void data_publish(String topic, String message){     //  資料上傳function
  debugLog("Sending data......");
  
  if(sta != 3) {
    // not ready for publishing
    return;
  }
  build_MQTT_connect();
  delay(3*1000);
  if(sta != 4) {
    // create connection failed
    return;
  }
  connect_MQTT();
  delay(3*1000);

  publish_MQTT(topic, message);
  
  close_MQTT();

}

void BC26_initail(){
  BC26_reset();
  delay(20*1000);       //  等待20秒連線
  ask_for_IP();         //  查詢IP狀況
  delay(2*1000);
  
}

void long_delay(int hr, int minut, int sec){        //  設定delay時間長度
  for(int i = 0; i < (hr*60*60 + minut*60 + sec); i++){
    delay(1000);
  }
}
