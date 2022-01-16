/*
 * zigbee.c
 *
 *  Created on: 16 gen 2022
 *      Author: anton
 */

#ifndef APPLICATION_USER_APP_ZIGBEE_C_
#define APPLICATION_USER_APP_ZIGBEE_C_

uint8_t mtZdoStateChangeIndCb(uint8_t newDevState);
uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg);
uint8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg);
uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg);
uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg);
uint8_t mtSysResetIndCb(ResetIndFormat_t *msg);
uint8_t mtSysVersionCb(VersionSrspFormat_t *msg);

#endif /* APPLICATION_USER_APP_ZIGBEE_C_ */
