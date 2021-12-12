/*
 * app.h
 *
 *  Created on: 11 dic 2021
 *      Author: anton
 */

#ifndef APPLICATION_USER_CORE_APPTASK_H_
#define APPLICATION_USER_CORE_APPTASK_H_

struct AppMessage
{
    char ucMessageID;
    char * content;
};

void vAppTask(void *pvParameters);

#endif
