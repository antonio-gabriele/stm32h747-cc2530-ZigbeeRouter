/*
 * app.h
 *
 *  Created on: 11 dic 2021
 *      Author: anton
 */

#ifndef APPLICATION_USER_CORE_APPTASK_H_
#define APPLICATION_USER_CORE_APPTASK_H_

#define MID_ZB_RESET_COO 		0x0
#define MID_ZB_RESET_RTR 		0x1
#define MID_ZB_ZBEE_START 		0x2
#define MID_ZB_ZBEE_SCAN 		0x3

#define MID_VW_LOG			0

struct AppMessage
{
    char ucMessageID;
    char content[256];
};

void vAppTask(void *pvParameters);
void vMsgTask(void *pvParameters);

#endif
