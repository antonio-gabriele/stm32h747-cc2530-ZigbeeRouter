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
ResetReqFormat_t const_hard_rst = { .Type = 0 };
uint8_t bsum = 0;
/********************************************************************************/
uint8_t app_show(const char *fmt, ...) {
	struct AppMessage msg;
	msg.ucMessageID = MID_VW_LOG;
	memset(msg.content, 0, sizeof(msg.content));
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg.content, 256, fmt, args);
	va_end(args);
	xQueueSend(xQueueBackendToView, (void* ) &msg, (TickType_t ) 0);
	return 0;
}

void app_reset(uint8_t devType) {
	uint8_t status = 0;
	OsalNvWriteFormat_t req = { .Id = ZCD_NV_STARTUP_OPTION, .Offset = 0, .Len = 1, .Value = { 0x03 } };
	status = sysOsalNvWrite(&req);
	status = sysResetReq(&const_hard_rst);
	vTaskDelay(4000);
	OsalNvWriteFormat_t nvWrite = { .Id = ZCD_NV_LOGICAL_TYPE, .Offset = 0, .Len = 1, .Value[0] = devType };
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
	sys_cfg.DeviceType = devType;
	sys_cfg.NodesCount = 0;
	cfgWrite();
}

void app_summary() {
	Summary_t summary = { 0 };
	zb_zdo_explore1(&summary);
	if (summary.nNodesAEOk < summary.nNodes || summary.nEndpointsSDOk < summary.nEndpoints) {
		xTimerStart(xTimer, 0);
	}
	if (bsum < (summary.nEndpointsSDOk + summary.nNodesAEOk)) {
		struct AppMessage message = { .ucMessageID = MID_APP_CFG_WRITE };
		xQueueSend(xQueueViewToBackend, (void* ) &message, (TickType_t ) 1000);
		app_show("D:%d/%d, E: %d/%d", summary.nNodesAEOk, summary.nNodes, summary.nEndpointsSDOk, summary.nEndpoints);
	}
	bsum = summary.nEndpointsSDOk + summary.nNodesAEOk;
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

void app_scanner() {
	MgmtLqiReqFormat_t req = { .DstAddr = 0, .StartIndex = 0 };
	ENQUEUE(MID_ZB_ZBEE_LQIREQ, MgmtLqiReqFormat_t, req);
	xTimerStart(xTimer, 0);
}

void app_start_stack() {
	uint8_t status = app_register_af();
	if (status != MT_RPC_SUCCESS) {
		printf("Register Af failed");
		return;
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
}

void vAppTaskLoop() {
	struct AppMessage xRxedStructure;
	if (xQueueReceive(xQueueViewToBackend, (struct AppMessage*) &xRxedStructure, (TickType_t) 10) == pdPASS) {
		switch (xRxedStructure.ucMessageID) {
		case MID_ZB_RESET_COO:
			app_reset(DEVICETYPE_COORDINATOR);
			break;
		case MID_ZB_RESET_RTR:
			app_reset(DEVICETYPE_ROUTER);
			break;
		case MID_ZB_ZBEE_START:
			app_start_stack();
			break;
		case MID_APP_CFG_WRITE:
			cfgWrite();
			break;
		case MID_ZB_ZBEE_SCAN:
			app_scanner();
			break;
		case MID_ZB_ZBEE_LQIREQ:
			DEQUEUE(MgmtLqiReqFormat_t, zdoMgmtLqiReq)
			break;
		case MID_ZB_ZBEE_ACTEND:
			DEQUEUE(ActiveEpReqFormat_t, zdoActiveEpReq)
			break;
		case MID_ZB_ZBEE_SIMDES:
			DEQUEUE(SimpleDescReqFormat_t, zdoSimpleDescReq)
			break;
		case MID_ZB_ZBEE_DATARQ:
			DEQUEUE(DataRequestFormat_t, afDataRequest)
		}
	}
	rpcWaitMqClientMsg(10);
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
	while (1)
		vAppTaskLoop();
}

void vPollTask(void *pvParameters) {
	while (1) {
		rpcWaitMqClientMsg(0xFFFF);
	}
}

void vComTask(void *pvParameters) {
	while (1) {
		rpcProcess();
		vTaskDelay(1);
	}
}

void vTimerCallback(TimerHandle_t xTimer) {
	app_summary();
}

void app_init() {
	rpcInitMq();
	rpcOpen();
	xQueueViewToBackend = xQueueCreate(20, sizeof(struct AppMessage));
	xQueueBackendToView = xQueueCreate(4, sizeof(struct AppMessage));
	xTimer = xTimerCreateStatic("Timer", pdMS_TO_TICKS(5000), pdFALSE, (void*) 0, vTimerCallback, &xTimerBuffer);
	xTaskCreate(vAppTask, "APP", 1024, NULL, 6, NULL);
	xTaskCreate(vComTask, "COM", 1024, NULL, 5, NULL);
	xTaskCreate(vPollTask, "POLL", 1024, NULL, 5, NULL);
}
