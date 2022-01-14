/*
 * app.h
 *
 *  Created on: 11 dic 2021
 *      Author: anton
 */



#ifndef APPLICATION_USER_CORE_APPTASK_H_
#define APPLICATION_USER_CORE_APPTASK_H_

#define MID_ZB_RESET_COO 		0x00
#define MID_ZB_RESET_RTR 		0x01
#define MID_ZB_ZBEE_START 		0x02
#define MID_ZB_ZBEE_SCAN 		0x03

#define MID_ZB_ZBEE_LQIREQ		0x04

#define MID_VW_LOG				0

struct AppMessage
{
    char ucMessageID;
    union {
    	char content[256];
    	//MgmtLqiReqFormat_t lqiReq;
    };
};

void vAppTask(void *pvParameters);
void vMsgTask(void *pvParameters);

#endif
