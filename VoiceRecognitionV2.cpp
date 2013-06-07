/**
  ******************************************************************************
  * @file    VoiceRecognitionV2.cpp
  * @author  Elechouse Team
  * @version V1.0
  * @date    2013-6-6
  * @brief   This file provides all the VoiceRecognitionV2 firmware functions.
  ******************************************************************************
    @note
         This driver is for elechouse Voice Recognition V2 Module(LINKS here)
  ******************************************************************************
  * @section  HISTORY
  
    V1.0    Initial version.

  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, ELECHOUSE SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2013 ELECHOUSE</center></h2>
  ******************************************************************************
  */
#include "VoiceRecognitionV2.h"
#include <string.h>

VR* VR::instance;

/** temp data buffer */
uint8_t vr_buf[32];
uint8_t hextab[17]="0123456789ABCDEF";

static char str[][16] PROGMEM = {
	"Success",
	"Failed",
};

VR::VR(uint8_t receivePin, uint8_t transmitPin) : SoftwareSerial(receivePin, transmitPin)
{
	instance = this;
	SoftwareSerial::begin(38400);
}

int VR :: recognize(uint8_t *buf, int timeout)
{
	int ret, i;
	ret = receive_pkt(vr_buf, timeout);
	if(vr_buf[2] != FRAME_CMD_VR){
		return -1;
	}
	if(ret > 0){
		for(i = 0; i < (vr_buf[1] - 3); i++){
			buf[i] = vr_buf[4+i];
		} 
		return i;
	}
	
	return 0;
}

int VR :: train(uint8_t record)
{
	int len;
	unsigned long start_millis;
	
	send_pkt(FRAME_CMD_TRAIN, &record, 1);
	start_millis = millis();
	while(1){
		len = receive_pkt(vr_buf);
		if(len>0){
			switch(vr_buf[2]){
				case FRAME_CMD_PROMPT:
					DBGSTR("Record:\t");
					DBGFMT(vr_buf[3], DEC);
					DBGSTR("\t");
					DBGBUF(vr_buf+4, len-4);
					break;
				case FRAME_CMD_TRAIN:
					DBGSTR("Train finish.\r\nSuccess: \t");
					DBGFMT(vr_buf[3], DEC);
					DBGSTR(" \r\n");
					return 0;
					break;
				default:
					break;
			}
			start_millis = millis();
		}
		if(millis()-start_millis > 4000){
			return -2;
		}
	}
	return 0;
}

int VR :: load(uint8_t *records, uint8_t len)
{
	uint8_t ret;
	send_pkt(FRAME_CMD_LOAD, records, len);
	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}
	if(vr_buf[2] != FRAME_CMD_LOAD){
		return -1;
	}
	DBGSTR("Success: \t");
	DBGFMT(vr_buf[3], DEC);
	DBGSTR("\r\n");
	for(ret = 0; ret < vr_buf[1]-3; ret += 2){
		DBGSTR("Record:\t");
		DBGFMT(vr_buf[4+ret], DEC);
		DBGSTR("\t");
		switch(vr_buf[4+ret+1]){
			case 0xFF:
				DBGSTR("Out Of Range");
				break;
			case 0xFE:
				DBGSTR("Untrained");
				break;
			case 0xFD:
				DBGSTR("VR Full");
				break;
			case 0xFC:
				DBGSTR("Already Loaded");
				break;
			case 0:
				DBGSTR("Success");
				break;
			default:
				DBGSTR("Unknown");
				break;
		}
		DBGLN("");
	}
	return 0;
}

int VR :: clear()
{	
	int len;
	send_pkt(FRAME_CMD_CLEAR, 0, 0);
	len = receive_pkt(vr_buf);
	if(len<=0){
		return -1;
	}

	if(vr_buf[2] != FRAME_CMD_CLEAR){
		return -1;
	}
	DBGLN("VR Module Cleared");
	return 0;
}

int VR :: test(uint8_t cmd, uint8_t *bsr)
{
	int len, i;
	unsigned long start_millis;
	switch(cmd){
		case FRAME_CMD_TEST_READ:
			vr_buf[0] = FRAME_CMD_TEST_READ;
			send_pkt(FRAME_CMD_TEST, vr_buf, 1);
			start_millis = millis();
			while(1){
				len = receive_pkt(vr_buf);
				if(len>0){
					switch(vr_buf[2]){
						case FRAME_CMD_TEST:
							memcpy(bsr+vr_buf[3]*20, vr_buf+4, 20);
							if(vr_buf[3] == 9){
								return 0;
							}
							break;
						default:
							DBGLN("TEST ERROR");
							return -1;
							break;
					}
					start_millis = millis();
				}
				if(millis()-start_millis > 4000){
					return -2;
				}
			}
			break;
		case FRAME_CMD_TEST_WRITE:
			for(i=0; i<10; i++){
				vr_buf[0] = i;
				memcpy(vr_buf+1, bsr+20*i, 20);
				send_pkt(FRAME_CMD_TEST, FRAME_CMD_TEST_WRITE, vr_buf, 21);
				start_millis = millis();
				while(1){
					len = receive_pkt(vr_buf);
					if(len>0){
						if(vr_buf[2] == FRAME_CMD_TEST){
							break;
						}else{
							DBGLN("TEST ERROR");
							return -1;
						}
						start_millis = millis();
					}
					if(millis()-start_millis > 4000){
						return -2;
					}
				}
			}
			break;
		default:
			break;
	}
	return 0;
}

int VR :: len(PROGMEM uint8_t *buf)
{
	int i=0;
	while(pgm_read_byte_near(buf++)){
		i++;
	}
	return i;
}

int VR :: cmp(uint8_t *buf, PROGMEM uint8_t *bufcmp, int len  )
{
	int i;
	for(i=0; i<len; i++){
		if(buf[i] != pgm_read_byte_near(bufcmp+i)){
			return -1;
		}
	}
	return 0;
}

void VR :: cpy(char *buf,  PROGMEM char * pbuf)
{
  int i=0;
  while(pgm_read_byte_near(pbuf)){
    buf[i] = pgm_read_byte_near(pbuf++);
    i++;
  }
}
int VR :: writehex(uint8_t *buf, uint8_t len)
{
	int i;
	for(i=0; i<len; i++){
		DBGCHAR(hextab[(buf[i]&0xF0)>>4]);
		DBGCHAR(hextab[(buf[i]&0x0F)]);
		DBGCHAR(' ');
	}
}

void VR :: send_pkt(uint8_t cmd, uint8_t subcmd, uint8_t *buf, uint8_t len)
{
	write(FRAME_HEAD);
	write(len+3);
	write(cmd);
	write(subcmd);
	write(buf, len);
	write(FRAME_END);
}

void VR :: send_pkt(uint8_t cmd, uint8_t *buf, uint8_t len)
{
	write(FRAME_HEAD);
	write(len+2);
	write(cmd);
	write(buf, len);
	write(FRAME_END);
}

int VR :: receive_pkt(uint8_t *buf, int timeout)
{
	int ret;
	ret = receive(buf, 2, timeout);
	if(ret != 2){
		return -1;
	}
	if(buf[0] != FRAME_HEAD){
		return -2;
	}
	if(buf[1] < 2){
		return -3;
	}
	ret = receive(buf+2, buf[1], timeout);
	if(buf[buf[1]+1] != FRAME_END){
		return -4;
	}
	
//	DBGBUF(buf, buf[1]+2);
	
	return buf[1]+2;
}

int VR::receive(uint8_t *buf, int len, int timeout)
{
  int read_bytes = 0;
  int ret;
  unsigned long start_millis;
  
  while (read_bytes < len) {
    start_millis = millis();
    do {
      ret = read();
      if (ret >= 0) {
        break;
     }
    } while( (millis()- start_millis ) < timeout);
    
    if (ret < 0) {
      return read_bytes;
    }
    buf[read_bytes] = (char)ret;
    read_bytes++;
  }
  
  return read_bytes;
}
