#ifndef _BCH26_H_
#define _BCH26_H_

#include <SoftwareSerial.h>

#define DEBUG_LEVEL 2


void BC26_initail();
void data_publish(String topic, String message);

extern void(* resetFunc) (void);

extern int time_count;
extern int reset_count;

extern SoftwareSerial mySerial;

#endif
