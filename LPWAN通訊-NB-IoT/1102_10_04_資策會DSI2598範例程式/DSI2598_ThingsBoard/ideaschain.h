#ifndef _IDEASCHAIN_H_
#define _IDEASCHAIN_H_

#include <Arduino.h>

extern const String clientId;       // MQTT client ID. it's better to use unique id.
extern const String username;       // device access token(存取權杖)
extern const String key;            // 使用者自訂的感測器名稱
extern const String topic;          // device topic

extern const String server_IP;      // ideaschain MQTT server ip
extern const int server_port;       // ideaschain MQTT server port

String message(String data);        // function to create MQTT message

#endif
