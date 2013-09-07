/**
  ******************************************************************************
  * @file    vr_sample_multi_cmd.ino
  * @author  JiapengLi
  * @brief   This file provides a demostration on 
              how to implement a multi voice command project (exceed 7 voice command) 
              by using VoiceRecognitionModule
  ******************************************************************************
  * @note:
        voice control led
  ******************************************************************************
  * @section  HISTORY
    
    2013/06/13    Initial version.
  */
  
#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"

/**        
  Connection
  Arduino    VoiceRecognitionModule
   2   ------->     TX
   3   ------->     RX
*/
VR myVR(2,3);    // 2:RX 3:TX, you can choose your favourite pins.

uint8_t record[7]; // save record
uint8_t buf[64];

int led = 13;

int group = 0;

#define switchRecord        (0)

#define group0Record1       (1) 
#define group0Record2       (2) 
#define group0Record3       (3) 
#define group0Record4       (4) 
#define group0Record5       (5) 
#define group0Record6       (6) 

#define group1Record1       (7) 
#define group1Record2       (8) 
#define group1Record3       (9) 
#define group1Record4       (10) 
#define group1Record5       (11) 
#define group1Record6       (12) 

void setup()
{
  /** initialize */
  myVR.begin(9600);
  
  Serial.begin(115200);
  Serial.println("Elechouse Voice Recognition V3 Module\r\nMulti Commands sample");
  
  pinMode(led, OUTPUT);
    
  if(myVR.clear() == 0){
    Serial.println("Recognizer cleared.");
  }else{
    Serial.println("Not find VoiceRecognitionModule.");
    Serial.println("Please check connection and restart Arduino.");
    while(1);
  }
  
  record[0] = switchRecord;
  record[1] = group0Record1;
  record[2] = group0Record2;
  record[3] = group0Record3;
  record[4] = group0Record4;
  record[5] = group0Record5;
  record[6] = group0Record6;
  group = 0;
  if(myVR.load(record, 7) >= 0){
    printRecord(record, 7);
    Serial.println(F("loaded."));
  }
  
}

void loop()
{
  int ret;
  ret = myVR.recognize(buf, 50);
  if(ret>0){
    switch(buf[1]){
      case switchRecord:
        /** turn on LED */
        if(digitalRead(led) == HIGH){
          digitalWrite(led, LOW);
        }else{
          digitalWrite(led, HIGH);
        }
        if(group == 0){
          group = 1;
          myVR.clear();
          record[0] = switchRecord;
          record[1] = group1Record1;
          record[2] = group1Record2;
          record[3] = group1Record3;
          record[4] = group1Record4;
          record[5] = group1Record5;
          record[6] = group1Record6;
          if(myVR.load(record, 7) >= 0){
            printRecord(record, 7);
            Serial.println(F("loaded."));
          }
        }else{
          group = 0;
          myVR.clear();
          record[0] = switchRecord;
          record[1] = group0Record1;
          record[2] = group0Record2;
          record[3] = group0Record3;
          record[4] = group0Record4;
          record[5] = group0Record5;
          record[6] = group0Record6;
          if(myVR.load(record, 7) >= 0){
            printRecord(record, 7);
            Serial.println(F("loaded."));
          }
        }
        break;
      default:
        break;
    }
    /** voice recognized */
    printVR(buf);
  }
}

/**
  @brief   Print signature, if the character is invisible, 
           print hexible value instead.
  @param   buf     --> command length
           len     --> number of parameters
*/
void printSignature(uint8_t *buf, int len)
{
  int i;
  for(i=0; i<len; i++){
    if(buf[i]>0x19 && buf[i]<0x7F){
      Serial.write(buf[i]);
    }
    else{
      Serial.print("[");
      Serial.print(buf[i], HEX);
      Serial.print("]");
    }
  }
}

/**
  @brief   Print signature, if the character is invisible, 
           print hexible value instead.
  @param   buf  -->  VR module return value when voice is recognized.
             buf[0]  -->  Group mode(FF: None Group, 0x8n: User, 0x0n:System
             buf[1]  -->  number of record which is recognized. 
             buf[2]  -->  Recognizer index(position) value of the recognized record.
             buf[3]  -->  Signature length
             buf[4]~buf[n] --> Signature
*/
void printVR(uint8_t *buf)
{
  Serial.println("VR Index\tGroup\tRecordNum\tSignature");

  Serial.print(buf[2], DEC);
  Serial.print("\t\t");

  if(buf[0] == 0xFF){
    Serial.print("NONE");
  }
  else if(buf[0]&0x80){
    Serial.print("UG ");
    Serial.print(buf[0]&(~0x80), DEC);
  }
  else{
    Serial.print("SG ");
    Serial.print(buf[0], DEC);
  }
  Serial.print("\t");

  Serial.print(buf[1], DEC);
  Serial.print("\t\t");
  if(buf[3]>0){
    printSignature(buf+4, buf[3]);
  }
  else{
    Serial.print("NONE");
  }
//  Serial.println("\r\n");
  Serial.println();
}

void printRecord(uint8_t *buf, uint8_t len)
{
  Serial.print(F("Record: "));
  for(int i=0; i<len; i++){
    Serial.print(buf[i], DEC);
    Serial.print(", ");
  }
}
