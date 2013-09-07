/**
 ******************************************************************************
 * @file    vr_sample_control_led.ino
 * @author  JiapengLi
 * @brief   This file provides a demostration on 
 *     checking the baud rate of VoiceRecognitionModule
 ******************************************************************************
 * @note:
 * check baud rate
 ******************************************************************************
 * @section  HISTORY
 * 
 * 2013/07/10    Initial version.
 */
#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"  
/**        
 * Connection
 * Arduino    VoiceRecognitionModule
 * 2   ------->     TX
 * 3   ------->     RX
 */
VR myVR(2,3);    // 2:RX 3:TX, you can choose your favourite pins.

int br[]={
  2400, 4800, 9600, 19200, 38400
};

void setup(void)
{
  /** initialize */
  int i=0;
  Serial.begin(115200);
  Serial.println("Elechouse Voice Recognition V3 Module\r\nCheck Baud Rate sample");
  for(i=0; i<5; i++){
    myVR.begin(br[i]);
    if(myVR.clear() == 0){
      Serial.print("Baud rate: ");
      Serial.println(br[i], DEC);
      break;
    }
  }
  if(i==5){
    Serial.println("Check baud rate failed. \r\nPlease check the connection, and reset arduino");
  }
}

void loop(void)
{

}

