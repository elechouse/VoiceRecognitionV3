/**
  ******************************************************************************
  * @file    vr_sample_bridge.ino
  * @author  JiapengLi
  * @brief   This file provides a demostration on 
              how to control led by using VoiceRecognitionModule
  ******************************************************************************
  * @note:
        Use this sample to try the command of VoiceRecognition Module.
    Eg:
       1. Enable Arduino Serial monitor "Send with newline" feture, Baud rate 115200.
       2. Input "01" to "check recognizer", 
       3. input "31" to "clear recognizer",
       4. input "30 00 02 04" to "load record 0, record 2, record 4"
  ******************************************************************************
  * @section  HISTORY
    
    2013/06/17    Initial version.
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

/***************************************************************************/
// command analyze part
#define CMD_BUF_LEN      64+1
uint8_t cmd[CMD_BUF_LEN];
uint8_t cmd_cnt;
uint8_t *paraAddr;

uint8_t buf[400];
uint8_t buflen[32];

void setup(void)
{
  myVR.begin(9600);
  
  /** initialize */
  Serial.begin(115200);
  Serial.println(F("Elechouse Voice Recognition V3 Module \"bridge\" sample."));
  Serial.println(F("Eg:\r\n1. Enable Arduino Serial monitor \"Send with newline\" feture, Baud rate 115200.\r\n2. Input \"01\" to \"check recognizer\".\r\n3. input \"31\" to \"clear recognizer\"\r\n4. input \"30 00 02 04\" to \"load record 0, record 2, record 4\""));
}

void loop(void)
{
  int len, i, ret, index;
  
  /** receive Serial command */
  len = receiveCMD();
  if(len > 0){
    printSeperator();  
    if(!checkCMD(len)){    
      ret = convertCMD(buf, len);
      if(ret>0){
        Serial.print("> ");
        myVR.writehex(buf, ret);
        myVR.send_pkt(buf, ret);
        Serial.println();
      }else{
        /** received command is invalid */
        /** display the receved command back */
        Serial.write(cmd, len);
        Serial.println(F("Input is not in hexadecimal format."));
      }    
    }else{
      /** received command is invalid */
      /** display the receved command back */
      Serial.print("> ");
      Serial.write(cmd, len); 
      Serial.println(F("Input is not in hexadecimal format."));
      
    }
//    printSeperator();
  }
  
  /** recieve all packet a time */
  len=0;
  index = 0;
  while(1){
    ret = myVR.receive_pkt(buf+len, 50);
    if(ret>0){
      len+=ret;
      buflen[index] = ret;
      index++;
    }else{
      break;
    }
  }
  if(index > 0){
    len = 0;
    for(i=0; i<index; i++){
      Serial.print("< ");
      myVR.writehex(buf+len, buflen[i]);
      len += buflen[i];
      Serial.println();
    }
  }
}

/**
  @brief   receive command from Serial.
  @param   NONE.
  @retval  command length, if no command receive return -1.
*/
int receiveCMD()
{
  int ret;
  int len;
  unsigned long start_millis;
  start_millis = millis();
  if(!Serial.available()){
    return -1;
  }
  while(1){
    ret = Serial.read();
    if(ret>0){
      start_millis = millis();
      cmd[cmd_cnt] = ret;
      if(cmd[cmd_cnt] == '\n'){
        len = cmd_cnt+1;
        cmd_cnt = 0;
        return len;
      }
      cmd_cnt++;
      if(cmd_cnt == CMD_BUF_LEN){
        cmd_cnt = 0;
        return -1;
      }
    }
    
    if(millis() - start_millis > 100){
      cmd_cnt = 0;
      return -1;
    }
  }
}

/**
  @brief   Check command format.
  @param   len  -->  command length
  @retval  0  -->  command is valid
          -1  -->  command is invalid
*/
int checkCMD(int len)
{
  int i;
  for(i=0; i<len; i++){
    if(cmd[i] >='0' && cmd[i] <= '9'){
    
    }else if(cmd[i] >='a' && cmd[i] <= 'f'){
    
    }else if(cmd[i] >='A' && cmd[i] <= 'Z'){
      
    }else if(cmd[i] == '\t' || cmd[i] == ' ' || cmd[i] == '\r' || cmd[i] == '\n'){
    
    }else{
      return -1;
    }
  }
  return 0;
}

/**
  @brief   Check the number of parameters in the command
  @param   len  -->  command length
  @retval  number of parameters
*/
int checkParaNum(int len)
{
  int cnt=0, i;
  for(i=0; i<len; ){
    if(cmd[i]!='\t' && cmd[i]!=' ' && cmd[i] != '\r' && cmd[i] != '\n'){
      cnt++;
      while(cmd[i] != '\t' && cmd[i] != ' ' && cmd[i] != '\r' && cmd[i] != '\n'){
        i++;
      }
    }
    i++;
  }
  return cnt;
}

/**
  @brief   Find the specified parameter.
  @param   len       -->  command length
           paraIndex -->  parameter index
           addr      -->  return value. position of the parameter
  @retval  length of specified parameter
*/
int findPara(int len, int paraIndex, uint8_t **addr)
{
  int cnt=0, i, paraLen;
  uint8_t dt;
  for(i=0; i<len; ){
    dt = cmd[i];
    if(dt!='\t' && dt!=' '){
      cnt++;
      if(paraIndex == cnt){
        *addr = cmd+i;
        paraLen = 0;
        while(cmd[i] != '\t' && cmd[i] != ' ' && cmd[i] != '\r' && cmd[i] != '\n'){
          i++;
          paraLen++;
        }
        return paraLen;
      }else{
        while(cmd[i] != '\t' && cmd[i] != ' ' && cmd[i] != '\r' && cmd[i] != '\n'){
          i++;
        }
      }
    }else{
      i++;
    }
  }
  return -1;
}

uint32_t atoh(uint8_t *str)
{
  int i, ret;
  i=0, ret = 0;
  while( (str[i]>='0' && str[i]<='9') || (str[i]>='A' && str[i]<='F') || (str[i]>='a' && str[i]<='f') ){
    if(i==8){
      ret = 0;
      break;
    }
    ret <<= 4;
    if(str[i]>='0' && str[i]<='9'){
      ret += (str[i]-'0');
    }else if(str[i]>='A' && str[i]<='F'){
      ret += (str[i] - 'A' + 0x0A);
    }else if(str[i]>='a' && str[i]<='f'){
      ret += (str[i] - 'a' + 0x0A);
    }
    i++;
  }
  
  return ret;
}

int convertCMD(uint8_t *des, int len)
{
  int i, paraNum, paraLen;
  paraNum = checkParaNum(len);
  for(i=0; i<paraNum; i++){
    paraLen = findPara(len , i+1, &paraAddr);
    if(paraLen>2){
      return -1;
    }
    des[i] = atoh(paraAddr);
    if(des[i] == 0){
      if(*paraAddr != '0'){
        return -1;
      }
      if(paraLen == 2 && *(paraAddr+1) != '0'){
        return -1;
      }
    }
  }
  return paraNum;
}

/**
  @brief   Print seperator. Print 80 '-'.
*/
void printSeperator()
{
  for(int i=0; i<80; i++){
    Serial.write('-');
  }
  Serial.println();
}

