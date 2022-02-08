#ifndef APPLICATION_USER_APP_ZIGBEE_C_
#define APPLICATION_USER_APP_ZIGBEE_C_

#include <mtParser.h>
#include <mtSys.h>
#include <mtZdo.h>
#include <stdint.h>
#include <stdbool.h>
#include <mtAf.h>
#include <application.h>
#include <mtUtil.h>

#define ZB1_OK 0xFF
#define ZB1_KO 0x00
#define ZB1_RE (0x02)
/********************************************************************************/
/********************************************************************************/
typedef union {
  uint64_t u64;
  uint8_t b8[sizeof(uint64_t)];
} IEEE;
/********************************************************************************/
/********************************************************************************/
void zbInit();
uint8_t zbStartScan(Fake_t*);
/********************************************************************************/
/********************************************************************************/
uint8_t zbRepair(Fake_t*);
uint8_t zbRepairNode(Node_t*, bool);
uint8_t zbRepairNodeEndpoint(Node_t*, Endpoint_t*);
uint8_t zbCount(Summary_t*);
/********************************************************************************/
/********************************************************************************/
uint8_t mtZdoStateChangeIndCb(uint8_t);
uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t*);
uint8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t*);
uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t*);
uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t*);
uint8_t mtSysResetIndCb(ResetIndFormat_t*);
uint8_t mtAfIncomingMsgCb(IncomingMsgFormat_t*);
uint8_t mtZdoIeeeAddrRspCb(IeeeAddrRspFormat_t*);
uint8_t mtZdoBindRspCb(BindRspFormat_t*);
uint8_t pfnUtilGetDeviceInfoCb(utilGetDeviceInfoFormat_t*);
Node_t* zbFindNodeByAddress(uint16_t);
Node_t* zbFindNodeByIEEE(uint64_t);
Endpoint_t* zbFindEndpoint(Node_t*, uint8_t);
#endif
