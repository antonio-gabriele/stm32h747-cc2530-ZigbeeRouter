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
/**/
extern Configuration_t sys_cfg;
/**/
QueueHandle_t xQueueViewToBackend;
QueueHandle_t xQueueBackendToView;
TimerHandle_t xTimer;
StaticTimer_t xTimerBuffer;
//
#define ENQUEUE(ID, STRUCTNAME, OBJECT) struct AppMessage message = { .ucMessageID = ID }; \
		memcpy(message.content, &OBJECT, sizeof(STRUCTNAME)); \
		vTaskDelay(100); \
		xQueueSend(xQueueViewToBackend, (void* ) &message, (TickType_t ) 0);

#define DEQUEUE(STRUCTNAME, FN) { STRUCTNAME req; \
			memccpy(&req, xRxedStructure.content, 1, sizeof(STRUCTNAME)); \
			FN(&req); }

devStates_t devState = DEV_HOLD;
ResetReqFormat_t const_hard_rst = { .Type = 0 };

Node_t* app_find_node_by_address(uint16_t address);
Endpoint_t* app_find_endpoint(Node_t *node, uint8_t endpoint);
void app_reset(uint8_t devType);
static uint8_t ui_show(const char *fmt, ...);

static uint8_t mtZdoStateChangeIndCb(uint8_t newDevState);
static uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg);
static uint8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg);
static uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg);
static uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg);
//SYS Callbacks
static uint8_t mtSysResetIndCb(ResetIndFormat_t *msg);
static uint8_t mtSysVersionCb(VersionSrspFormat_t *msg);
static uint8_t ui_show(const char *fmt, ...);

extern QueueHandle_t xQueueViewToBackend;

static mtSysCb_t mtSysCb = {
NULL,
NULL,
NULL, mtSysResetIndCb, mtSysVersionCb,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL };

static mtZdoCb_t mtZdoCb = {
NULL,       // MT_ZDO_NWK_ADDR_RSP
		NULL,      // MT_ZDO_IEEE_ADDR_RSP
		NULL,      // MT_ZDO_NODE_DESC_RSP
		NULL,     // MT_ZDO_POWER_DESC_RSP
		mtZdoSimpleDescRspCb,    // MT_ZDO_SIMPLE_DESC_RSP
		mtZdoActiveEpRspCb,      // MT_ZDO_ACTIVE_EP_RSP
		NULL,     // MT_ZDO_MATCH_DESC_RSP
		NULL,   // MT_ZDO_COMPLEX_DESC_RSP
		NULL,      // MT_ZDO_USER_DESC_RSP
		NULL,     // MT_ZDO_USER_DESC_CONF
		NULL,    // MT_ZDO_SERVER_DISC_RSP
		NULL, // MT_ZDO_END_DEVICE_BIND_RSP
		NULL,          // MT_ZDO_BIND_RSP
		NULL,        // MT_ZDO_UNBIND_RSP
		NULL,   // MT_ZDO_MGMT_NWK_DISC_RSP
		mtZdoMgmtLqiRspCb,       // MT_ZDO_MGMT_LQI_RSP
		NULL,       // MT_ZDO_MGMT_RTG_RSP
		NULL,      // MT_ZDO_MGMT_BIND_RSP
		NULL,     // MT_ZDO_MGMT_LEAVE_RSP
		NULL,     // MT_ZDO_MGMT_DIRECT_JOIN_RSP
		NULL,     // MT_ZDO_MGMT_PERMIT_JOIN_RSP
		mtZdoStateChangeIndCb,   // MT_ZDO_STATE_CHANGE_IND
		mtZdoEndDeviceAnnceIndCb,   // MT_ZDO_END_DEVICE_ANNCE_IND
		NULL,        // MT_ZDO_SRC_RTG_IND
		NULL,	 //MT_ZDO_BEACON_NOTIFY_IND
		NULL,			 //MT_ZDO_JOIN_CNF
		NULL,	 //MT_ZDO_NWK_DISCOVERY_CNF
		NULL,                    // MT_ZDO_CONCENTRATOR_IND_CB
		NULL,         // MT_ZDO_LEAVE_IND
		NULL,   //MT_ZDO_STATUS_ERROR_RSP
		NULL,  //MT_ZDO_MATCH_DESC_RSP_SENT
		NULL, NULL };

static uint8_t mtSysResetIndCb(ResetIndFormat_t *msg) {
	printf("Reset ZNP Version: %d.%d.%d\n", msg->MajorRel, msg->MinorRel, msg->HwRev);
	return 0;
}

static uint8_t mtSysVersionCb(VersionSrspFormat_t *msg) {
	printf("Request ZNP Version: %d.%d.%d\n", msg->MajorRel, msg->MinorRel, msg->Product);
	return 0;
}

static uint8_t ui_show(const char *fmt, ...) {
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
		ui_show("Errore");
	}
	//
	status = sysResetReq(&const_hard_rst);
	if (status != 0) {
		ui_show("Errore");
	}
	vTaskDelay(4000);
	sys_cfg.DeviceType = devType;
	sys_cfg.NodesCount = 0;
	cfgWrite();
}

/////////////////////////////////////////////////

/********************************************************************/

static uint8_t mtZdoStateChangeIndCb(uint8_t newDevState) {

	switch (newDevState) {
	case DEV_HOLD:
		ui_show("Hold");
		break;
	case DEV_INIT:
		ui_show("Init");
		break;
	case DEV_NWK_DISC:
		ui_show("Network Discovering");
		break;
	case DEV_NWK_JOINING:
		ui_show("Network Joining");
		break;
	case DEV_NWK_REJOIN:
		ui_show("Network Rejoining");
		break;
	case DEV_END_DEVICE_UNAUTH:
		ui_show("Network Authenticating");
		break;
	case DEV_END_DEVICE:
		ui_show("Network Joined as End Device");
		break;
	case DEV_ROUTER:
		ui_show("Network Joined as Router");
		break;
	case DEV_COORD_STARTING:
		ui_show("Network Starting");
		break;
	case DEV_ZB_COORD:
		ui_show("Network Started");
		break;
	case DEV_NWK_ORPHAN:
		ui_show("Network Orphaned");
		break;
	default:
		ui_show("Unknown state");
		break;
	}

	devState = (devStates_t) newDevState;

	return SUCCESS;
}

static uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg) {
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	xTimerReset(xTimer, 0);
	if (msg->StartIndex + msg->NeighborLqiListCount < msg->NeighborTableEntries) {
		MgmtLqiReqFormat_t req = { .DstAddr = msg->SrcAddr, .StartIndex = msg->StartIndex + msg->NeighborLqiListCount };
		ENQUEUE(MID_ZB_ZBEE_LQIREQ, MgmtLqiReqFormat_t, req)
	}
	if (msg->StartIndex + msg->NeighborLqiListCount == msg->NeighborTableEntries) {
		Node_t *node1 = app_find_node_by_address(msg->SrcAddr);
		node1->LqiCompleted = 0xFF;
	}
	uint32_t i = 0;
	for (i = 0; i < msg->NeighborLqiListCount; i++) {
		uint16_t address = msg->NeighborLqiList[i].NetworkAddress;
		Node_t *node1 = app_find_node_by_address(address);
		if (node1 != NULL) {
			return msg->Status;
		}
		printf("mtZdoMgmtLqiRspCb -> Found: %04X, Count: %d\n", address, sys_cfg.NodesCount);
		uint8_t devType = msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat & 3;
		Node_t *node = &sys_cfg.Nodes[sys_cfg.NodesCount];
		sys_cfg.NodesCount++;
		node->LqiCompleted = 0x00;
		node->ActiveEndpointCompleted = 0x00;
		node->Address = address;
		node->Type = devType;
		{
			ActiveEpReqFormat_t req = { .DstAddr = address, .NwkAddrOfInterest = address };
			ENQUEUE(MID_ZB_ZBEE_ACTEND, ActiveEpReqFormat_t, req);
		}
		if (devType == DEVICETYPE_ROUTER) {
			MgmtLqiReqFormat_t req = { .DstAddr = address, .StartIndex = 0 };
			ENQUEUE(MID_ZB_ZBEE_LQIREQ, MgmtLqiReqFormat_t, req);
		}
		break;
	}
	return msg->Status;
}

static uint8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg) {
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	xTimerReset(xTimer, 0);
	printf("mtZdoSimpleDescRspCb -> %04X:%d, In: %d\n", msg->NwkAddr, msg->Endpoint, msg->NumInClusters);
	Node_t *node = app_find_node_by_address(msg->NwkAddr);
	Endpoint_t *endpoint = app_find_endpoint(node, msg->Endpoint);
	endpoint->SimpleDescriptorCompleted = 0xFF;
	endpoint->InClusterCount = msg->NumInClusters;
	endpoint->OutClusterCount = msg->NumOutClusters;
	uint32_t i;
	for (i = 0; i < msg->NumInClusters; i++) {
		endpoint->InClusters[i].Cluster = msg->InClusterList[i];
	}
	for (i = 0; i < msg->NumOutClusters; i++) {
		endpoint->OutClusters[i].Cluster = msg->OutClusterList[i];
	}
	return msg->Status;
}

static uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg) {
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	xTimerReset(xTimer, 0);
	Node_t *node = app_find_node_by_address(msg->SrcAddr);
	node->ActiveEndpointCompleted = 0xFF;
	node->EndpointCount = msg->ActiveEPCount;
	printf("mtZdoActiveEpRspCb -> Address: %04X, Endpoints: %d\n", msg->SrcAddr, msg->ActiveEPCount);
	uint32_t i;
	for (i = 0; i < msg->ActiveEPCount; i++) {
		uint8_t endpoint = msg->ActiveEPList[i];
		node->Endpoints[i].Endpoint = endpoint;
		node->Endpoints[i].SimpleDescriptorCompleted = 0x00;
		SimpleDescReqFormat_t req = { .DstAddr = msg->NwkAddr, .NwkAddrOfInterest = msg->NwkAddr, .Endpoint = endpoint };
		ENQUEUE(MID_ZB_ZBEE_SIMDES, SimpleDescReqFormat_t, req)
	}
	return msg->Status;

}

static uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg) {
	printf("Joined: 0x%04X\n", msg->NwkAddr);
	ActiveEpReqFormat_t req = { .DstAddr = msg->NwkAddr, .NwkAddrOfInterest = msg->NwkAddr };
	ENQUEUE(MID_ZB_ZBEE_ACTEND, ActiveEpReqFormat_t, req);
	return 0x00;
}

/************************************************************************/
static int32_t app_register_af(void) {
	int32_t status = 0;
	RegisterFormat_t reg = { .EndPoint = 1, .AppProfId = 0x0104, //
			.AppDeviceId = 0x0100, .AppDevVer = 1, //
			.LatencyReq = 0, .AppNumInClusters = 1, //
			.AppInClusterList[0] = 0x0006, .AppNumOutClusters = 0 };
	status = afRegister(&reg);
	return status;
}

Endpoint_t* app_find_endpoint(Node_t *node, uint8_t endpoint) {
	uint8_t i1;
	for (i1 = 0; i1 < node->EndpointCount; ++i1) {
		if (node->Endpoints[i1].Endpoint == endpoint) {
			return &node->Endpoints[i1];
		}
	}
	return NULL;
}

Node_t* app_find_node_by_address(uint16_t address) {
	uint8_t i1;
	for (i1 = 0; i1 < sys_cfg.NodesCount; ++i1) {
		if (sys_cfg.Nodes[i1].Address == address) {
			return &sys_cfg.Nodes[i1];
		}
	}
	return NULL;
}

void app_scanner() {
	Node_t *coo = &sys_cfg.Nodes[0];
	coo->Address = 0;
	coo->ActiveEndpointCompleted = 0x00;
	coo->LqiCompleted = 0x00;
	coo->Type = 0;
	sys_cfg.NodesCount = 1;
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
		ui_show("Start new network");
		status = MT_RPC_SUCCESS;
	} else if (status == RESTORED_NETWORK) {
		ui_show("Restored network");
		status = MT_RPC_SUCCESS;
	} else {
		ui_show("Start failed");
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
		}
	}
	rpcWaitMqClientMsg(10);
}

void vAppTask(void *pvParameters) {
	cfgRead();
	sysRegisterCallbacks(mtSysCb);
	zdoRegisterCallbacks(mtZdoCb);
	znp_if_init();
	znp_cmd_init();
	vTaskDelay(1000);
	uint8_t status = 0;
	status = sysResetReq(&const_hard_rst);
	do {
		vTaskDelay(1000);
		status = sysVersion();
	} while (status != 0);
	ui_show("System started");
	while (1)
		vAppTaskLoop();
}

void vPollTask(void *pvParameters) {
	while (1) {
		rpcWaitMqClientMsg(0xFFFF);
	}
}

void vComTask(void *pvParameters) {
	xTaskCreate(vPollTask, "POLL", 1024, NULL, 5, NULL);
	while (1) {
		rpcProcess();
		vTaskDelay(1);
	}
}

void vTimerCallback(TimerHandle_t xTimer) {
	uint8_t b0 = 0, b1 = 0;
	uint8_t i0 = 0, i1 = 0;
	uint8_t m0 = 0, m1 = 0;
	for (i0 = 0; i0 < sys_cfg.NodesCount; i0++) {
		b0++;
		if (sys_cfg.Nodes[i0].ActiveEndpointCompleted == 0x00) {
			ActiveEpReqFormat_t req = { //
					.DstAddr = sys_cfg.Nodes[i0].Address, //
					.NwkAddrOfInterest = sys_cfg.Nodes[i0].Address };
			ENQUEUE(MID_ZB_ZBEE_ACTEND, ActiveEpReqFormat_t, req);
		}
		if (sys_cfg.Nodes[i0].ActiveEndpointCompleted == 0xFF) {
			m0++;
			for (i1 = 0; i1 < sys_cfg.Nodes[i0].EndpointCount; i1++) {
				b1++;
				if (sys_cfg.Nodes[i0].Endpoints[i1].SimpleDescriptorCompleted == 0x00) {
					SimpleDescReqFormat_t req = { //
							.DstAddr = sys_cfg.Nodes[i0].Address, //
							.NwkAddrOfInterest = sys_cfg.Nodes[i0].Address, //
							.Endpoint = sys_cfg.Nodes[i0].Endpoints[i1].Endpoint };
					ENQUEUE(MID_ZB_ZBEE_SIMDES, SimpleDescReqFormat_t, req)
				}
				if (sys_cfg.Nodes[i0].Endpoints[i1].SimpleDescriptorCompleted == 0xFF) {
					m1++;
				}
			}

		}
	}
	if(m0 < b0){
		xTimerStart(xTimer, 0);
	}
	ui_show("D:%d/%d, E: %d/%d", m0, b0, m1, b1);
}

void app_init() {
	rpcInitMq();
	rpcOpen();
	xQueueViewToBackend = xQueueCreate(32, sizeof(struct AppMessage));
	xQueueBackendToView = xQueueCreate(8, sizeof(struct AppMessage));
	xTimer = xTimerCreateStatic("Timer", pdMS_TO_TICKS(5000), pdFALSE, (void*) 0, vTimerCallback, &xTimerBuffer);
	xTaskCreate(vAppTask, "APP", 2048, NULL, 6, NULL);
	xTaskCreate(vComTask, "COM", 1024, NULL, 5, NULL);
}
