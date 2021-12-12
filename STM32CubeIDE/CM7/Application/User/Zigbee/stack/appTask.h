/*
 * app.h
 *
 *  Created on: 11 dic 2021
 *      Author: anton
 */

#ifndef APPLICATION_USER_CORE_APPTASK_H_
#define APPLICATION_USER_CORE_APPTASK_H_

#define MID_ZB_START 		0
#define MID_ZB_PAIR 		1

#define MID_VW_LOG			0

struct AppMessage
{
    char ucMessageID;
    char content[256];
};

void vAppTask(void *pvParameters);
void vMsgTask(void *pvParameters);

#endif
