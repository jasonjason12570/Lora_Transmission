#include <Arduino.h>
#include "ideaschain.h"

const String clientId = "Ideaschain";               // MQTT client ID. it's better to use unique id.
const String username = "IL9l1nnNTUZUsLRdHaGd";        // device access token(存取權杖)
const String key = "PM2.5";                            // 使用者自訂的感測器名稱(可隨意)
const String topic = "\"v1/devices/me/telemetry\"";    // Fixed topic. ***DO NOT MODIFY***

const String server_IP = "iiot.ideaschain.com.tw";
const int server_port = 1883;

String data_tempt = "";

String message(String data){
  data_tempt = "\"{\"";
  data_tempt.concat(key);
  data_tempt.concat("\":");
  data_tempt.concat(data);
  data_tempt.concat("}\"");

  return data_tempt;
}
