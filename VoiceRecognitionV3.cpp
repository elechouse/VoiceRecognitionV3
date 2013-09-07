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
#include "VoiceRecognitionV3.h"
#include <string.h>

VR* VR::instance;

/** temp data buffer */
uint8_t vr_buf[32];
uint8_t hextab[17]="0123456789ABCDEF";

/**
	@brief VR class constructor.
	@param receivePin --> software serial RX
		   transmitPin --> software serial TX
*/
VR::VR(uint8_t receivePin, uint8_t transmitPin) : SoftwareSerial(receivePin, transmitPin)
{
	instance = this;
	SoftwareSerial::begin(38400);
}

/**
	@brief VR class constructor.
	@param buf --> return data .
			 buf[0]  -->  Group mode(FF: None Group, 0x8n: User, 0x0n:System
             buf[1]  -->  number of record which is recognized. 
             buf[2]  -->  Recognizer index(position) value of the recognized record.
             buf[3]  -->  Signature length
             buf[4]~buf[n] --> Signature
		   timeout --> wait time for receiving packet.
	@retval length of valid data in buf. 0 means no data received.
*/
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

/**
	@brief train records, at least one.
	@param records --> record data buffer pointer.
		   len --> number of records.
           buf --> pointer of return value buffer, optional.
		   buf[0]  -->  number of records which are trained successfully.
             buf[2i+1]  -->  record number
             buf[2i+2]  -->  record train status.
                00 --> Trained 
                FE --> Train Time Out
                FF --> Value out of range"
             (i = 0 ~ len-1 )
	@retval '>0' --> length of valid data in buf. 
            0 --> success, and no data received.
            '<0' --> failed.
                -1 --> data format error.
                -2 --> train timeout.
*/
int VR :: train(uint8_t *records, uint8_t len, uint8_t *buf)
{
	int ret;
	unsigned long start_millis;
	if(len == 0){
		return -1;
	}
	
	send_pkt(FRAME_CMD_TRAIN, records, len);
	start_millis = millis();
	while(1){
		ret = receive_pkt(vr_buf);
		if(ret>0){
			switch(vr_buf[2]){
				case FRAME_CMD_PROMPT:
					DBGSTR("Record:\t");
					DBGFMT(vr_buf[3], DEC);
					DBGSTR("\t");
					DBGBUF(vr_buf+4, ret-4);
					break;
				case FRAME_CMD_TRAIN:
					if(buf != 0){
						memcpy(buf, vr_buf+3, vr_buf[1]-2);
						return vr_buf[1]-2;
					}
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
		if(millis()-start_millis > 8000){
			return -2;
		}
	}
	return 0;
}

/**
	@brief train one record.
	@param records --> record data buffer pointer.
		   len --> number of records.
           buf --> pointer of return value buffer, optional.
		   buf[0]  -->  number of records which are trained successfully.
             buf[2i+1]  -->  record number
             buf[2i+2]  -->  record train status.
                00 --> Trained 
                FE --> Train Time Out
                FF --> Value out of range"
             (i = 0 ~ len-1 )
	@retval '>0' --> length of valid data in buf. 
            0 --> success, and no data received.
            '<0' --> failed.
                -1 --> data format error.
                -2 --> train timeout.
*/
int VR :: train(uint8_t record, uint8_t *buf)
{
    return train(&record, 1, buf);
}

/**
	@brief train record and set a signature(alias) for this record.
	@param record --> record value.
           buf --> signature string/data pointer.
		   len --> lenght of buf.
		   retbuf --> return data .
			 retbuf[0]  -->  number of records which are trained successfully.
             retbuf[1]  -->  record number.
             retbuf[2]  -->  record train status.
                00 --> Trained 
                F0 --> Trained, signature truncate
                FE --> Train Time Out
                FF --> Value out of range"
             retbuf[3] ~ retbuf[retval-1] --> Signature.(retval means return value)
	@retval '>0' --> length of valid data in buf. 
            0 --> success, and no data received.
            '<0' --> failed.
                -1 --> data format error.
                -2 --> train with signature timeout.
*/
int VR :: trainWithSignature(uint8_t record, const void *buf, uint8_t len, uint8_t * retbuf)
{
	int ret;
	unsigned long start_millis;
	if(len){
		send_pkt(FRAME_CMD_SIG_TRAIN, record, (uint8_t *)buf, len);
	}else{
		if(buf == 0){
			return -1;
		}
		len = strlen((char *)buf);
		if(len>10){
			return -1;
		}
		send_pkt(FRAME_CMD_SIG_TRAIN, record, (uint8_t *)buf, len);
	}
	
	start_millis = millis();
	while(1){
		ret = receive_pkt(vr_buf);
		if(ret>0){
			switch(vr_buf[2]){
				case FRAME_CMD_PROMPT:
					DBGSTR("Record:\t");
					DBGFMT(vr_buf[3], DEC);
					DBGSTR("\t");
					DBGBUF(vr_buf+4, ret-4);
					break;
				case FRAME_CMD_SIG_TRAIN:
					if(retbuf != 0){
						memcpy(retbuf, vr_buf+3, vr_buf[1]-2);
						return vr_buf[1]-2;
					}
				
					DBGSTR("Train finish.\r\nSuccess: \t");
					DBGFMT(vr_buf[3], DEC);
					DBGSTR(" \r\n");
					writehex(vr_buf, vr_buf[1]+2);
					return 0;
					break;
				default:
					break;
			}
			start_millis = millis();
		}
		if(millis()-start_millis > 8000){
			return -2;
		}
	}
	return 0;
}

/**
    @brief Load records to recognizer.
    @param records --> record data buffer pointer.
           len --> number of records.
           buf --> pointer of return value buffer, optional.
             buf[0]    -->  number of records which are load successfully.
             buf[2i+1]  -->  record number
             buf[2i+2]  -->  record load status.
                00 --> Loaded 
                FC --> Record already in recognizer
                FD --> Recognizer full
                FE --> Record untrained
                FF --> Value out of range"
             (i = 0 ~ '(retval-1)/2' )
	@retval '>0' --> length of valid data in buf. 
            0 --> success, buf=0, and no data returned.
            '<0' --> failed.
*/
int VR :: load(uint8_t *records, uint8_t len, uint8_t *buf)
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
	if(buf != 0){
		memcpy(buf, vr_buf+3, vr_buf[1]-2);
		return vr_buf[1]-2;
	}
	return 0;
}

/**
    @brief Load one record to recognizer.
    @param record --> record value.
           buf --> pointer of return value buffer, optional.
             buf[0]    -->  number of records which are load successfully.
             buf[2i+1]  -->  record number
             buf[2i+2]  -->  record load status.
                00 --> Loaded 
                FC --> Record already in recognizer
                FD --> Recognizer full
                FE --> Record untrained
                FF --> Value out of range"
             (i = 0 ~ '(retval-1)/2' )
	@retval '>0' --> length of valid data in buf. 
            0 --> success, buf=0, and no data returned.
            '<0' --> failed.
*/
int VR :: load(uint8_t record, uint8_t *buf)
{
	uint8_t ret;
	send_pkt(FRAME_CMD_LOAD, &record, 1);
	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}
	if(vr_buf[2] != FRAME_CMD_LOAD){
		return -1;
	}
	if(buf != 0){
		memcpy(buf, vr_buf+3, vr_buf[1]-2);
		return vr_buf[1]-2;
	}
	return 0;
}

/**
    @brief set signature(alias) for a record.
    @param record --> record value.
           buf --> signature buffer.
           len --> length of buf.
    @retval 0 --> success, buf=0, and no data returned.
            '<0' --> failed.
*/
int VR :: setSignature(uint8_t record, const void *buf, uint8_t len)
{
	int ret;
	
	if(len == 0 && buf == 0){
		/** delete signature */
	}else if(len == 0 && buf != 0){
		if(buf == 0){
			return -1;
		}
		len = strlen((char *)buf);
		if(len>10){
			return -1;
		}
	}else if(len != 0 && buf != 0){
		
	}else{
		return -1;
	}
	send_pkt(FRAME_CMD_SET_SIG, record, (uint8_t *)buf, len);
	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}
	if(vr_buf[2] != FRAME_CMD_SET_SIG){
		return -1;
	}
	return 0;
}

/**
    @brief delete signature(alias) of a record.
    @param record --> record value.
    @retval  0 --> success
            -1 --> failed
*/
int VR :: deleteSignature(uint8_t record)
{
    return setSignature(record);
}

/**
    @brief check the signature(alias) of a record.
    @param record --> record value.
           buf --> signature, return value buffer.
    @retval '>0' --> length of valid data in buf. 
            0 --> success, buf=0, and no data returned.
            '<0' --> failed.
*/
int VR :: checkSignature(uint8_t record, uint8_t *buf)
{
	int ret;
	if(record < 0){
		return -1;
	}
	send_pkt(FRAME_CMD_CHECK_SIG, record, 0, 0);
	ret = receive_pkt(vr_buf);
	
	if(ret<=0){
		return -1;
	}
	if(vr_buf[2] != FRAME_CMD_CHECK_SIG){
		return -1;
	}

	if(vr_buf[4]>0){
		memcpy(buf, vr_buf+5, vr_buf[4]);
		return vr_buf[4];
	}else{
		return 0;
	}
}

/**
    @brief clear recognizer.
    @retval  0 --> success
            -1 --> failed
*/
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
	//DBGLN("VR Module Cleared");
	return 0;
}

/**
    @brief clear recognizer.
    @param buf --> return value buffer.
             buf[0]     -->  Number of valid voice records in recognizer
             buf[i+1]   -->  Record number.(0xFF: Not loaded(Nongroup mode), or not set (Group mode)) 
                (i= 0, 1, ... 6)
             buf[8]     -->  Number of all voice records in recognizer
             buf[9]     -->  Valid records position indicate.
             buf[10]    -->  Group mode indicate(FF: None Group, 0x8n: User, 0x0n:System
    @retval '>0' --> success, length of data in buf 
            -1 --> failed
*/
int VR :: checkRecognizer(uint8_t *buf)
{
	int len;
	send_pkt(FRAME_CMD_CHECK_BSR, 0, 0);
	len = receive_pkt(vr_buf);
	if(len<=0){
		return -1;
	}

	if(vr_buf[2] != FRAME_CMD_CHECK_BSR){
		return -1;
	}
	
	if(vr_buf[1] != 0x0D){
		return -1;
	}
	
	memcpy(buf, vr_buf+3, vr_buf[1]-2);
	
	return vr_buf[1]-2;
}

/**
    @brief check record train status.
    @param buf --> return value
             buf[0]     -->  Number of checked records
             buf[2i+1]  -->  Record number.
             buf[2i+2]  -->  Record train status. (00: untrained, 01: trained, FF: record value out of range)
             (i = 0 ~ buf[0]-1 )
    @retval Number of trained records
*/
int VR :: checkRecord(uint8_t *buf, uint8_t *records, uint8_t len)
{
	int ret;
	int cnt = 0;
	unsigned long start_millis;
	if(records == 0 && len==0){
        memset(buf, 0xF0, 255);
		send_pkt(FRAME_CMD_CHECK_TRAIN, 0xFF, 0, 0);
		start_millis = millis();
		while(1){
			len = receive_pkt(vr_buf);
			if(len>0){
				if(vr_buf[2] == FRAME_CMD_CHECK_TRAIN){
                    for(int i=0; i<vr_buf[1]-3; i+=2){
                        buf[vr_buf[4+i]]=vr_buf[4+i+1];
                    }
					cnt++;
					if(cnt == 51){
						return vr_buf[3];
					}
				}else{
					return -3;
				}
				start_millis = millis();
			}
			
			if(millis()-start_millis > 500){
				if(cnt>0){
					buf[0] = cnt*5;
					return vr_buf[3];
				}
				return -2;
			}
			
		}
		
	}else if(len>0){
		ret = cleanDup(vr_buf, records, len);
		send_pkt(FRAME_CMD_CHECK_TRAIN, vr_buf, ret);
		ret = receive_pkt(vr_buf);
		if(ret>0){
			if(vr_buf[2] == FRAME_CMD_CHECK_TRAIN){
				memcpy(buf+1, vr_buf+4, vr_buf[1]-3);
				buf[0] = (vr_buf[1]-3)/2;
				return vr_buf[3];
			}else{
				return -3;
			}
		}else{
			return -1;
		}
	}else{
		return -1;
	}
	
}

/****************************************************************************/
/******************************* GROUP CONTROL ******************************/
/**
    @brief set group control by external IO function
    @param ctrl --> group control by external IO
                0 --> disable group control by external IO
                1 --> user group control by external IO
                2 --> system group control by external IO
    @retval  0 --> success
            -1 --> failed
*/
int VR :: setGroupControl(uint8_t ctrl)
{
	int ret;
	if(ctrl>2){
		return -1;
	}
	
	send_pkt(FRAME_CMD_GROUP, FRAME_CMD_GROUP_SET, &ctrl, 1);
	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}

	if(vr_buf[2] != FRAME_CMD_GROUP){
		return -1;
	}
	return 0;
}

/**
    @brief check group control by external IO function
    @param ctrl --> group control by external IO
    @retval  0 --> group control by external IO disabled
             1 --> user group control by external IO status
             2 --> system group control by external IO status
            -1 --> failed
*/
int VR :: checkGroupControl()
{
	uint8_t cmd;
	int ret;
	cmd = 0xFF;
	send_pkt(FRAME_CMD_GROUP, FRAME_CMD_GROUP_SET, &cmd, 1);
	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}

	if(vr_buf[2] != FRAME_CMD_GROUP){
		return -1;
	}
	ret = vr_buf[5];
	if(ret == 0xFF){
		ret = 0;
	}
	return ret;
}

/**
    @brief set user gruop content.
    @param grp --> user group number.
           records --> pointer of records buffer.
           len --> length of reocrds
    @retval  0 --> success
            -1 --> failed
*/
int VR :: setUserGroup(uint8_t grp, uint8_t *records, uint8_t len)
{
	int ret;
	if(len == 0 || records == 0){
		return -1;
	}
	if(grp >= 8){
		return -1;
	}
	vr_buf[0] = grp;
	memcpy(vr_buf+1, records, len);
	send_pkt(FRAME_CMD_GROUP, FRAME_CMD_GROUP_SUGRP, vr_buf, len+1);
	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}
	if(vr_buf[2] != FRAME_CMD_GROUP){
		return -1;
	}
	return 0;
}

/**
    @brief check user gruop content.
    @param grp --> user group number.
           buf --> return value
             buf[8i]    -->  group number.
             buf[8i+1]  -->  group position 0 status.
             buf[8i+2]  -->  group position 1 status.
               ...                ...
             buf[8i+6]  -->  group position 5 status.
             buf[8i+7]  -->  group position 6 status.
             (i = 0 ~ @retval)
    @retval '>0' --> number of checked user group
            '<0' --> failed
*/
int VR :: checkUserGroup(uint8_t grp, uint8_t *buf)
{
	int ret;
	int cnt = 0;
	unsigned long start_millis;
	
	if(grp == GROUP_ALL){
		send_pkt(FRAME_CMD_GROUP, FRAME_CMD_GROUP_CUGRP, 0, 0);
		start_millis = millis();
		while(1){
			ret = receive_pkt(vr_buf);
			if(ret>0){
				if(vr_buf[2] == FRAME_CMD_GROUP && vr_buf[1] == 10){
					memcpy(buf+8*cnt, vr_buf+3, vr_buf[1]-2);
					cnt++;
					if(cnt == 8){
						return cnt;
					}
				}else{
					return -3;
				}
				start_millis = millis();
			}
			
			if(millis()-start_millis > 500){
				if(cnt>0){
					return cnt;
				}
				return -2;
			}
			
		}
	}else if(grp <= GROUP7){
		send_pkt(FRAME_CMD_GROUP, FRAME_CMD_GROUP_CUGRP, &grp, 1);
		ret = receive_pkt(vr_buf);
		if(ret>0){
			if(vr_buf[2] == FRAME_CMD_GROUP && vr_buf[1] == 10){
				memcpy(buf+8*cnt, vr_buf+3, vr_buf[1]-2);
				return 1;
			}else{
				return -3;
			}
		}else{
			return -2;
		}
	}else{
		return -1;
	}
}

/**
    @brief load system gruop content to recognizer.
    @param grp --> syestem group number.
           buf  -->  return value.
             buf[0]     -->  Number of valid voice records in recognizer.
             buf[i+1]   -->  Record number.(0xFF: Not loaded(Nongroup mode), or not set (Group mode)) 
                (i= 0, 1, ... 6)
             buf[8]     -->  Number of all voice records in recognizer
             buf[9]     -->  Valid records position indicate.
             buf[10]    -->  Group mode indicate(FF: None Group, 0x8n: User, 0x0n:System
             (i = 0 ~ @retval)
    @retval '>0' --> length of buf
            '<0' --> failed
*/
int VR :: loadSystemGroup(uint8_t grp, uint8_t *buf)
{
	int ret;
	if(grp > 10){
		return -1;
	}
	send_pkt(FRAME_CMD_GROUP, FRAME_CMD_GROUP_LSGRP, &grp, 1);
	ret = receive_pkt(vr_buf);
	
	if(ret <= 0){
		return -1;
	}
	
	if(vr_buf[2] != FRAME_CMD_GROUP){
		return -1;
	}
	
	if(buf != 0){
		vr_buf[3] = 0;
		for(int i=0; i<8; i++){
			if(vr_buf[12]&(1<<i)){
				vr_buf[3]++;
			}
		}
		memcpy(buf, vr_buf+3, vr_buf[1]-2);
		return vr_buf[1]-2;
	}
	
	return 0;
}

/**
    @brief load user gruop content to recognizer.
    @param grp --> user group number.
           buf  -->  return value.
             buf[0]     -->  Number of valid voice records in recognizer.
             buf[i+1]   -->  Record number.(0xFF: Not loaded(Nongroup mode), or not set (Group mode)) 
                (i= 0, 1, ... 6)
             buf[8]     -->  Number of all voice records in recognizer
             buf[9]     -->  Valid records position indicate.
             buf[10]    -->  Group mode indicate(FF: None Group, 0x8n: User, 0x0n:System)
             (i = 0 ~ @retval)
    @retval '>0' --> length of buf
            '<0' --> failed
*/
int VR :: loadUserGroup(uint8_t grp, uint8_t *buf)
{
	int ret;
	if(grp > GROUP7){
		return -1;
	}
	send_pkt(FRAME_CMD_GROUP, FRAME_CMD_GROUP_LUGRP, &grp, 1);
	ret = receive_pkt(vr_buf);
	
	if(ret <= 0){
		return -1;
	}
	
	if(vr_buf[2] != FRAME_CMD_GROUP){
		return -1;
	}
	
	if(buf != 0){
		vr_buf[3] = 0;
		for(int i=0; i<8; i++){
			if(vr_buf[12]&(1<<i)){
				vr_buf[3]++;
			}
		}
		memcpy(buf, vr_buf+3, 11);
		return 1;
	}
	
	return 0;
}

/**
    @brief reset system setting to default
    @retval  0 --> success
            -1 --> failed
*/
int VR :: restoreSystemSettings()
{
	int len;
	send_pkt(FRAME_CMD_RESET_DEFAULT, 0, 0);
	len = receive_pkt(vr_buf);
	if(len<=0){
		return -1;
	}

	if(vr_buf[2] != FRAME_CMD_RESET_DEFAULT){
		return -1;
	}

	return 0;
}

/**
    @brief check system settings
    @param buf --> return value
            buf[0] --> baud rate. (0-9600 1-2400 2-4800 3-9600 4-19200 5-38400)
            buf[1] --> output io mode(0-pulse 1-toggle 2-clear 3-set)
            buf[2] --> pulse width level
            buf[3] --> auto load(0,0xFF-disable 1-enable)
            buf[4] --> Group control by external IO(0-disable 1-system group 2-user group)
    @retval '>0' --> buf length
            -1 --> failed
*/
int VR :: checkSystemSettings(uint8_t* buf)
{
	int len;
	if(buf == 0){
		return -1;
	}
	send_pkt(FRAME_CMD_CHECK_SYSTEM, 0, 0);
	len = receive_pkt(vr_buf);
	if(len<=0){
		return -1;
	}
	
	if(vr_buf[2] != FRAME_CMD_CHECK_SYSTEM){
		return -1;
	}

	memcpy(buf, vr_buf+4, vr_buf[1]-3);
	return vr_buf[1]-3;

	return 0;
}

/**
    @brief set module baud rate.
    @param br --> module baud rate.(0-9600 1-2400 2-4800 3-9600 4-19200 5-38400)
    @retval 0 --> success
            -1 --> failed
*/
int VR :: setBaudRate(unsigned long br)
{
	uint8_t baud_rate;
	int ret;
	switch(br){
		case 2400:
			baud_rate = 1;
			break;
		case 4800:
			baud_rate = 2;
			break;
		case 9600:
			baud_rate = 0;
			//baud_rate = 3;
			break;
		case 19200:
			baud_rate = 4;
			break;
		case 38400:
			baud_rate = 5;
			break;
		default:
			return -1;
			break;
	}
	
	send_pkt(FRAME_CMD_SET_BR, baud_rate, 0, 0);
	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}

	if(vr_buf[2] != FRAME_CMD_SET_BR){
		return -1;
	}
	//DBGLN("VR Module Cleared");
	return 0;
	
}

/**
    @brief set module output IO mode.
    @param mode --> module output IO mode.(must be PULSE, TOGGLE, SET, CLEAR)
    @retval 0 --> success
            -1 --> failed
*/
int VR :: setIOMode(io_mode_t mode)
{
	if(mode > 3){
		return -1;
	}
	int ret;
	
	send_pkt(FRAME_CMD_SET_IOM, mode, 0, 0);
	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}

	if(vr_buf[2] != FRAME_CMD_SET_IOM){
		return -1;
	}
	return 0;
}

/**
    @brief resset module output IO.
    @param ios --> output IO buffer.
           len --> length of ios.
    @retval 0 --> success
            -1 --> failed
*/
int VR :: resetIO(uint8_t *ios, uint8_t len)
{
	int ret;
	if(len == 1 && ios == 0){
		send_pkt(FRAME_CMD_RESET_IO, 0xFF, 0, 0);
	}else if(len != 0 && ios != 0){
		send_pkt(FRAME_CMD_RESET_IO, ios, len);
	}else{
		return -1;
	}

	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}
	if(vr_buf[2] != FRAME_CMD_RESET_IO){
		return -1;
	}
	return 0;
}

/**
    @brief set module pulse width(PULSE mode).
    @param level --> pulse width level.(LEVEL0~LEVEL15)
           len --> length of ios.
    @retval 0 --> success
            -1 --> failed
*/
int VR :: setPulseWidth(uint8_t level)
{
	int ret;
	
	if(level > VR::LEVEL15){
		return -1;
	}
	
	send_pkt(FRAME_CMD_SET_PW, level, 0, 0);
	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}

	if(vr_buf[2] != FRAME_CMD_SET_PW){
		return -1;
	}
	return 0;
}

/**
    @brief set autoload.
    @param records --> record buffer.
           len --> records length.
    @retval 0 --> success
            -1 --> failed
*/
int VR :: setAutoLoad(uint8_t *records, uint8_t len)
{
	int ret;
	uint8_t map;
	if(len == 0 && records == 0){
		map = 0;
	}else if(len != 0 && records != 0){
		map = 0;
		for(int i=0; i<len; i++){
			map |= (1<<i);
		}
	}else{
		return -1;
	}
	
	send_pkt(FRAME_CMD_SET_AL, map, records, len);
	ret = receive_pkt(vr_buf);
	if(ret<=0){
		return -1;
	}
	if(vr_buf[2] != FRAME_CMD_SET_AL){
		return -1;
	}
	return 0;
}

/**
    @brief disable autoload.
    @param records --> record buffer.
           len --> records length.
    @retval 0 --> success
            -1 --> failed
*/
int VR :: disableAutoLoad()
{
    return setAutoLoad();
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

/**flash operation function (strlen)*/
int VR :: len(uint8_t *buf)
{
	int i=0;
	while(pgm_read_byte_near(buf++)){
		i++;
	}
	return i;
}

/**flash operation function (strcmp)*/
int VR :: cmp(uint8_t *buf, uint8_t *bufcmp, int len  )
{
	int i;
	for(i=0; i<len; i++){
		if(buf[i] != pgm_read_byte_near(bufcmp+i)){
			return -1;
		}
	}
	return 0;
}

/**flash operation function (strcpy)*/
void VR :: cpy(char *buf, char * pbuf)
{
  int i=0;
  while(pgm_read_byte_near(pbuf)){
    buf[i] = pgm_read_byte_near(pbuf++);
    i++;
  }
}

/** ascending sort */
void VR :: sort(uint8_t *buf, int len)
{
	int i, j;
	uint8_t tmp;
	for(i=0; i<len; i++){
		for(j=i+1; j<len; j++){
			if(buf[j] < buf[i]){
				tmp = buf[i];
				buf[i] = buf[j]; 
				buf[j] = tmp;
			}
		}
	}
}

/** remove duplicates */
int VR :: cleanDup(uint8_t *des, uint8_t *buf, int len)
{	
	if(len<1){
		return -1;
	}
	
	int i, j, k=0;
	for(i=0; i<len; i++){
		for(j=0; j<k; j++){
			if(buf[i] == des[j]){
				break;
			}
		}
		if(j==k){
			des[k] = buf[i];
			k++;
		}
	}
	return k;
#if 0
	int i=0, j=1, k=0;
	sort(buf, len);
	for(; j<len; ){
		des[k] = buf[i];
		k++;
		while(buf[i]==buf[j]){
			j++;
			if(j==len){
				return k;
			}
		}
		i=j;
	}
	return k;
#endif
}

/** Serial print in HEX format */
int VR :: writehex(uint8_t *buf, uint8_t len)
{
	int i;
	for(i=0; i<len; i++){
		DBGCHAR(hextab[(buf[i]&0xF0)>>4]);
		DBGCHAR(hextab[(buf[i]&0x0F)]);
		DBGCHAR(' ');
	}
	return len;
}

/**
    @brief send data packet in Voice Recognition module protocol format.
    @param cmd --> command 
           subcmd --> subcommand
           buf --> data area
           len --> length of buf
*/
void VR :: send_pkt(uint8_t cmd, uint8_t subcmd, uint8_t *buf, uint8_t len)
{
	while(available()){
		flush();
	}
	write(FRAME_HEAD);
	write(len+3);
	write(cmd);
	write(subcmd);
	write(buf, len);
	write(FRAME_END);
}

/**
    @brief send data packet in Voice Recognition module protocol format.
    @param cmd --> command 
           buf --> data area
           len --> length of buf
*/
void VR :: send_pkt(uint8_t cmd, uint8_t *buf, uint8_t len)
{
	while(available()){
		flush();
	}
	write(FRAME_HEAD);
	write(len+2);
	write(cmd);
	write(buf, len);
	write(FRAME_END);
}

/**
    @brief send data packet in Voice Recognition module protocol format.
    @param buf --> data area
           len --> length of buf
*/
void VR :: send_pkt(uint8_t *buf, uint8_t len)
{
	while(available()){
		flush();
	}
	write(FRAME_HEAD);
	write(len+1);
	write(buf, len);
	write(FRAME_END);
}

/**
    @brief receive a valid data packet in Voice Recognition module protocol format.
    @param buf --> return value buffer.
           timeout --> time of reveiving
    @retval '>0' --> success, packet lenght(length of all data in buf)
            '<0' --> failed
*/
int VR :: receive_pkt(uint8_t *buf, uint16_t timeout)
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

/**
    @brief receive data .
    @param buf --> return value buffer.
           len --> length expect to receive.
           timeout --> time of reveiving
    @retval number of received bytes, 0 means no data received.
*/
int VR::receive(uint8_t *buf, int len, uint16_t timeout)
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
