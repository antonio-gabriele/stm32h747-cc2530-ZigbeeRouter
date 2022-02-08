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
#include <FreeRTOS.h>
#include <queue.h>
#include <system.h>
#include "cmsis_os.h"
#include "rpc.h"
#include <timers.h>
#include <zigbee.h>
/**/
/********************************************************************************/
/********************************************************************************/
#define QUEUE_LENGTH    64
#define STACK_SIZE 		2048
#define ITEM_SIZE       sizeof( struct AppMessage )
/********************************************************************************/
/********************************************************************************/
#define QUEUEUI_LENGTH 4
#define ITEMUI_SIZE sizeof(Tuple2_t)
static StaticQueue_t xStaticQueueUI;
static uint8_t ucQueueStorageAreaUI[QUEUEUI_LENGTH * ITEMUI_SIZE];
/********************************************************************************/
/********************************************************************************/
TimerHandle_t xTimer;
static StaticTimer_t xTimerBuffer;
static StaticTask_t xTaskBufferPoll;
static StackType_t xStackPoll[STACK_SIZE];
static StaticTask_t xTaskBufferApp;
static StackType_t xStackApp[STACK_SIZE];
static StaticTask_t xTaskBufferCom;
static StackType_t xStackCom[STACK_SIZE];
static StaticQueue_t xStaticQueue;
static uint8_t ucQueueStorageArea[QUEUE_LENGTH * ITEM_SIZE];
/********************************************************************************/
/********************************************************************************/
Configuration_t sys_cfg = { 0 };
QueueHandle_t xQueueUI;
QueueHandle_t xQueue;
utilGetDeviceInfoFormat_t system;
ResetReqFormat_t const_hard_rst = { .Type = 0 };
uint8_t bsum = 0;
uint8_t machineState = 0;
/********************************************************************************/
void call_C_displayMessage(char *message);
void call_C_refreshNodeEndpoint(Node_t *node, Endpoint_t *endpoint);
/********************************************************************************/
uint8_t appPrintf(const char *fmt, ...) {
	char content[64];
	va_list args;
	va_start(args, fmt);
	vsnprintf(content, 64, fmt, args);
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
		appPrintf("Errore");
	}
	status = sysResetReq(&const_hard_rst);
	if (status != 0) {
		appPrintf("Errore");
	}
	vTaskDelay(4000);
	sys_cfg.DeviceType = devType->u8;
	sys_cfg.NodesCount = 0;
	return MT_RPC_SUCCESS;
}

uint8_t appRepair() {
	zbRepair(NULL);
	return MT_RPC_SUCCESS;
}

uint8_t appCount() {
	Summary_t summary = { 0 };
	zbCount(&summary);
	uint8_t thumb = summary.nEndpoints + //
			summary.nEndpointsSDOk + //
			summary.nEndpointsBindOk + //
			summary.nEndpointsValueOk + //
			summary.nNodesNameOk + //
			summary.nNodesAEOk + //
			summary.nNodesLQOk + //
			summary.nNodes;
	if (bsum != thumb) {
		appPrintf("ND:%d/AE:%d/LQ:%d/ID:%d-EP:%d/SD:%d/BN:%d/VL:%d", //
				summary.nNodes, //
				summary.nNodesAEOk, //
				summary.nNodesLQOk, //
				summary.nNodesNameOk, //
				summary.nEndpoints, //
				summary.nEndpointsSDOk, //
				summary.nEndpointsBindOk, //
				summary.nEndpointsValueOk);
	}
	bsum = thumb;
	return MT_RPC_SUCCESS;

}

uint8_t appTick() {
	xTimerReset(xTimer, 0);
	appRepair();
	appCount();
	return MT_RPC_SUCCESS;
}

static int32_t app_register_af(void) {
	int32_t status = 0;
	RegisterFormat_t reg = {
			.EndPoint = 1, //
			.AppProfId = 0x0104, //
			.AppDeviceId = 0x0100, //
			.AppDevVer = 1, //
			.LatencyReq = 0, //
			.AppNumInClusters = 1, //
			.AppInClusterList[0] = 0x0006, //
			.AppNumOutClusters = 1, //
			.AppOutClusterList[0] = 0x0006 };
	status = afRegister(&reg);
	return status;
}

uint8_t app_scanner(void *none) {
	xTimerReset(xTimer, 0);
	Fake_t req;
	zbStartScan(&req);
	return MT_RPC_SUCCESS;
}

uint8_t appStartStack(void *none) {
	uint8_t status = app_register_af();
	status = zdoInit();
	if (status == NEW_NETWORK) {
		appPrintf("Start new network");
		status = MT_RPC_SUCCESS;
	} else if (status == RESTORED_NETWORK) {
		appPrintf("Restored network");
		status = MT_RPC_SUCCESS;
	} else {
		appPrintf("Start failed");
		status = -1;
	}
	machineState = 2;
	Fake_t req;
	RUN(utilGetDeviceInfo, req)
	return MT_RPC_SUCCESS;
}

void vAppTaskLoop() {
	struct AppMessage xRxedStructure;
	if (xQueueReceive(xQueue, (struct AppMessage*) &xRxedStructure, (TickType_t) portMAX_DELAY) == pdPASS) {
		if (xRxedStructure.fn != NULL) {
			void (*fn)(void*) = xRxedStructure.fn;
			fn(xRxedStructure.params);
		}
	}
}

void vAppTask(void *pvParameters) {
	zbInit();
	vTaskDelay(1000);
	machineState = 1;
	sysResetReq(&const_hard_rst);
	while (1)
		vAppTaskLoop();
}

void vPollTask(void *pvParameters) {
	while (1) {
		rpcWaitMqClientMsg(1);
	}
}

void vComTask(void *pvParameters) {
	rpcInitMq();
	rpcOpen();
	xTaskCreateStatic(vPollTask, "POLL", STACK_SIZE, NULL, 5, xStackPoll, &xTaskBufferPoll);
	while (1) {
		rpcProcess();
		vTaskDelay(1);
	}
}

void vTimerCallback(TimerHandle_t xTimer) {
	appTick();
}

uint8_t app_init(void *none) {
	xQueueUI = xQueueCreateStatic(QUEUEUI_LENGTH, ITEMUI_SIZE, ucQueueStorageAreaUI, &xStaticQueueUI);
	xQueue = xQueueCreateStatic(QUEUE_LENGTH, ITEM_SIZE, ucQueueStorageArea, &xStaticQueue);
	xTimer = xTimerCreateStatic("Timer", pdMS_TO_TICKS(1000), pdFALSE, (void*) 0, vTimerCallback, &xTimerBuffer);
	xTaskCreateStatic(vAppTask, "APP", STACK_SIZE, NULL, tskIDLE_PRIORITY, xStackApp, &xTaskBufferApp);
	xTaskCreateStatic(vComTask, "COM", STACK_SIZE, NULL, 6, xStackCom, &xTaskBufferCom);
	return MT_RPC_SUCCESS;
}
