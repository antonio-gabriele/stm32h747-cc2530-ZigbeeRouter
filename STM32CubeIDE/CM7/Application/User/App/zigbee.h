#ifndef APPLICATION_USER_APP_ZIGBEE_C_
#define APPLICATION_USER_APP_ZIGBEE_C_

#include <mtParser.h>
#include <mtSys.h>
#include <mtZdo.h>
#include <stdint.h>
#include <mtAf.h>
#include <application.h>
#include <mtUtil.h>

#define ZB_OK 0x00
#define ZB_KO 0xFF
#define ZB_RE (0xFF-0x04)

uint8_t zb_zdo_explore1(Summary_t*);
uint8_t zb_zdo_explore(Node_t*, Summary_t*);
uint8_t mtZdoStateChangeIndCb(uint8_t);
uint8_t zbZdoMgmtLqiReq(MgmtLqiRspFormat_t*);
uint8_t zb_zdo_simple_descriptor(SimpleDescRspFormat_t*);
uint8_t zb_zdo_end_device_announce(EndDeviceAnnceIndFormat_t*);
uint8_t zbZdoActiveEpReq(ActiveEpRspFormat_t*);
uint8_t mtSysResetIndCb(ResetIndFormat_t*);
uint8_t zb_sys_version(VersionSrspFormat_t*);
uint8_t zb_af_incoming_msg(IncomingMsgFormat_t*);
uint8_t zdoIeeeAddrReqCb(IeeeAddrRspFormat_t*);
uint8_t zb_zdo_bind(BindRspFormat_t*);
uint8_t pfnUtilGetDeviceInfoCb(utilGetDeviceInfoFormat_t*);
Node_t* zb_find_node_by_address(uint16_t);
Node_t* zb_find_node_by_ieee(uint64_t);
Endpoint_t* zb_find_endpoint(Node_t*, uint8_t);
Cluster_t* zb_find_cluster(Endpoint_t*, uint16_t);
void zb_init();

#endif
