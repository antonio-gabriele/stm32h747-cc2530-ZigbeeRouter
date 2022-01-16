#ifndef APPLICATION_USER_APP_ZIGBEE_C_
#define APPLICATION_USER_APP_ZIGBEE_C_

#include <mtParser.h>
#include <mtSys.h>
#include <mtZdo.h>
#include <stdint.h>

uint8_t zb_zdo_state_changed(uint8_t newDevState);
uint8_t zb_zdo_mgmt_remote_lqi(MgmtLqiRspFormat_t *msg);
uint8_t zb_zdo_simple_descriptor(SimpleDescRspFormat_t *msg);
uint8_t zb_zdo_end_device_announce(EndDeviceAnnceIndFormat_t *msg);
uint8_t zb_zdo_active_endpoint(ActiveEpRspFormat_t *msg);
uint8_t zb_sys_reset(ResetIndFormat_t *msg);
uint8_t zb_sys_version(VersionSrspFormat_t *msg);

void zb_init();

#endif
