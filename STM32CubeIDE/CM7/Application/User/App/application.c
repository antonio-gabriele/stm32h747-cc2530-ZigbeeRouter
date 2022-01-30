#include <application.h>
#include "cmsis_os.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dbgPrint.h"
#include "mtAf.h"
#include "mtAppCfg.h"
#include "mtParser.h"
#include "mtSys.h"
#include "mtUtil.h"
#include "mtZdo.h"
#include "rpc.h"
#include "rpcTransport.h"
#include "znp_cmd.h"
#include "znp_if.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <system.h>
#include "cmsis_os.h"
#include "rpc.h"
#include <queue.h>
#include <timers.h>
#include <zigbee.h>
/**/
Configuration_t sys_cfg = { 0 };
QueueHandle_t xQueueViewToBackend;
QueueHandle_t xQueueBackendToView;
TimerHandle_t xTimer;
StaticTimer_t xTimerBuffer;
utilGetDeviceInfoFormat_t system;
ResetReqFormat_t const_hard_rst = { .Type = 0 };
uint8_t bsum = 0;
/********************************************************************************/
void call_C_displayMessage(char *message);
/********************************************************************************/
uint8_t app_show(const char *fmt, ...) {
	char content[64];
	va_list args;
	va_start(args, fmt);
	vsnprintf(content, 256, fmt, args);
	va_end(args);
	call_C_displayMessage(content);
	return 0;
}

uint8_t app_reset(Fake_t *devType) {
	uint8_t status = 0;
	OsalNvWriteFormat_t req = { .Id = ZCD_NV_STARTUP_OPTION, .Offset = 0, .Len = 1, .Value = { 0x03 } };
	status = sysOsalNvWrite(&req);
	status = sysResetReq(&const_hard_rst);
	vTaskDelay(4000);
	OsalNvWriteFormat_t nvWrite = { .Id = ZCD_NV_LOGICAL_TYPE, .Offset = 0, .Len = 1, .Value[0] = devType->u8 };
	status = sysOsalNvWrite(&nvWrite);
	if (status != 0) {
		app_show("Errore");
	}
	//
	status = sysResetReq(&const_hard_rst);
	if (status != 0) {
		app_show("Errore");
	}
	vTaskDelay(4000);
	sys_cfg.DeviceType = devType->u8;
	sys_cfg.NodesCount = 0;
	cfgWrite();
	return 0;
}

uint8_t app_summary(void *none) {
	Summary_t summary = { 0 };
	zb_zdo_explore1(&summary);
	uint8_t thumb = summary.nEndpoints + summary.nEndpointsSDOk + summary.nNodesNameOk + summary.nNodesAEOk + summary.nNodesIEEEOk + summary.nNodesLQOk;
	//if (summary.nNodesNameOk < summary.nNodes || summary.nEndpointsBindOk < summary.nEndpoints) {
	xTimerStart(xTimer, 0);
	//}
	if (bsum != thumb) {
		void *req = 0;
		RUN(cfgWrite, req)
		app_show("ND:%d/AE:%d/IE:%d/LQ:%d/ID:%d-EP:%d/SD:%d/BN:%d", //
				summary.nNodes, //
				summary.nNodesAEOk, //
				summary.nNodesIEEEOk, //
				summary.nNodesLQOk, //
				summary.nNodesNameOk, //
				summary.nEndpoints, //
				summary.nEndpointsSDOk, //
				summary.nEndpointsBindOk);
	}
	bsum = thumb;
	return 0;
}

static int32_t app_register_af(void) {
	int32_t status = 0;
	RegisterFormat_t reg = { .EndPoint = 1, .AppProfId = 0x0104, //
			.AppDeviceId = 0x0100, .AppDevVer = 1, //
			.LatencyReq = 0, .AppNumInClusters = 1, //
			.AppInClusterList[0] = 0x0006, .AppNumOutClusters = 0 };
	status = afRegister(&reg);
	return status;
}

uint8_t app_scanner(void *none) {
	utilGetDeviceInfo();
	xTimerStart(xTimer, 0);
	return 0;
}

uint8_t app_start_stack(void *none) {
	uint8_t status = app_register_af();
	if (status != MT_RPC_SUCCESS) {
		printf("Register Af failed");
		return 1;
	}
	status = zdoInit();
	if (status == NEW_NETWORK) {
		app_show("Start new network");
		status = MT_RPC_SUCCESS;
	} else if (status == RESTORED_NETWORK) {
		app_show("Restored network");
		status = MT_RPC_SUCCESS;
	} else {
		app_show("Start failed");
		status = -1;
	}
	return status;
}

void vAppTaskLoop() {
	struct AppMessage xRxedStructure;
	if (xQueueReceive(xQueueViewToBackend, (struct AppMessage*) &xRxedStructure, (TickType_t) 10) == pdPASS) {
		if (xRxedStructure.fn != NULL) {
			void (*fn)(void*) = xRxedStructure.fn;
			fn(xRxedStructure.params);
		}
	}
}

void vAppTask(void *pvParameters) {
	cfgRead();
	zb_init();
	znp_if_init();
	znp_cmd_init();
	vTaskDelay(1000);
	uint8_t status = 0;
	status = sysResetReq(&const_hard_rst);
	do {
		vTaskDelay(1000);
		status = sysVersion();
	} while (status != 0);
	app_show("System started");
	Fake_t req;
	RUN(app_start_stack, req)
	while (1)
		vAppTaskLoop();
}

void vComTask(void *pvParameters) {
	while (1) {
		rpcProcess();
		rpcWaitMqClientMsg(10);
	}
}

void vTimerCallback(TimerHandle_t xTimer) {
	if (system.ieee_addr == 0) {
		utilGetDeviceInfo();
	} else {
		app_summary(NULL);
	}
}

uint8_t app_init(void *none) {
	rpcInitMq();
	rpcOpen();
	xQueueViewToBackend = xQueueCreate(16, sizeof(struct AppMessage));
	xQueueBackendToView = xQueueCreate(4, sizeof(struct AppMessage));
	xTimer = xTimerCreateStatic("Timer", pdMS_TO_TICKS(2000), pdFALSE, (void*) 0, vTimerCallback, &xTimerBuffer);
	xTaskCreate(vAppTask, "APP", 1024, NULL, 8, NULL);
	xTaskCreate(vComTask, "COM", 1024, NULL, 6, NULL);
	return 0;
}
