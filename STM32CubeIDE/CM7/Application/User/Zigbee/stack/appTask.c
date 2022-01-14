#include <appTask.h>
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
#include <configuration.h>
/**/
extern my_Configuration sys_cfg;
/**/
extern QueueHandle_t xQueueBackendToView;

#define MAX_CHILDREN 10
#define MAX_NODE_LIST 10
//
devStates_t devState = DEV_HOLD;
ResetReqFormat_t const_hard_rst = { .Type = 0 };
//init ZDO device state
uint8_t gSrcEndPoint = 1;
uint8_t gDstEndPoint = 1;

typedef struct {
	uint16_t ChildAddr;
	uint8_t Type;

} ChildNode_t;

typedef struct {
	uint8_t Status;
	uint16_t NodeAddr;
	uint8_t Type;
	uint8_t ChildCount;
	ChildNode_t childs[MAX_CHILDREN];
} Node_t;

Node_t nodeList[MAX_NODE_LIST];
uint8_t nodeCount = 0;
//
static int32_t startNetwork1(void);
//ZDO Callbacks
static uint8_t mtZdoStateChangeIndCb(uint8_t newDevState);
static uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg);
static uint8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg);
static uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg);
static uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg);
//SYS Callbacks
static uint8_t mtSysResetIndCb(ResetIndFormat_t *msg);
static uint8_t mtSysVersionCb(VersionSrspFormat_t *msg);

//MY
static uint8_t show(const char *fmt, ...);

//helper functions
static uint8_t setNVStartup(uint8_t startupOption);
static uint8_t setNVChanList(uint32_t chanList);
static uint8_t setNVPanID(uint32_t panId);
static uint8_t setNVDevType(uint8_t devType);
static int32_t startNetwork(void);
static int32_t registerAf(void);

extern QueueHandle_t xQueueViewToBackend;

// SYS callbacks
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
	show("Reset ZNP Version: %d.%d.%d", msg->MajorRel, msg->MinorRel,
			msg->HwRev);
	return 0;
}

static uint8_t mtSysVersionCb(VersionSrspFormat_t *msg) {
	show("Request ZNP Version: %d.%d.%d", msg->MajorRel, msg->MinorRel,
			msg->Product);
	return 0;
}

static uint8_t show(const char *fmt, ...) {
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

void reset(uint8_t devType) {
	uint8_t status = 0;
	OsalNvWriteFormat_t req = { .Id = ZCD_NV_STARTUP_OPTION, .Offset = 0, .Len =
			1, .Value = { 0x03 } };
	status = sysOsalNvWrite(&req);
	status = sysResetReq(&const_hard_rst);
	vTaskDelay(4000);
	/*
	 if(status != 0){
	 show("Errore");
	 }
	 //

	 //
	 req.Value[0] = 0;
	 status = sysOsalNvWrite(&req);
	 if(status != 0){
	 show("Errore");
	 }
	 */
	OsalNvWriteFormat_t nvWrite = { .Id = ZCD_NV_LOGICAL_TYPE, .Offset = 0,
			.Len = 1, .Value[0] = devType };
	status = sysOsalNvWrite(&nvWrite);
	if (status != 0) {
		show("Errore");
	}
	//
	status = sysResetReq(&const_hard_rst);
	if (status != 0) {
		show("Errore");
	}
	vTaskDelay(4000);
	sys_cfg.devType = devType;
	cfgWrite();
}

void resetCoo() {
	reset(DEVICETYPE_COORDINATOR);
}

void resetRtr() {
	reset(DEVICETYPE_ROUTER);
}

void scan() {
	MgmtLqiReqFormat_t lqiReq = { .DstAddr = 0, .StartIndex = 0 };
	zdoMgmtLqiReq(&lqiReq);
	/*
	struct AppMessage msg = { .ucMessageID = MID_ZB_ZBEE_LQIREQ };
	memcpy(msg.content, &lqiReq, sizeof(MgmtLqiReqFormat_t));
	xQueueSend(xQueueViewToBackend, (void* ) &msg, (TickType_t ) 0);
	*/
}

void start() {
	show("Starting");
	uint8_t status = registerAf();
	if (status != MT_RPC_SUCCESS) {
		show("Register Af failed");
		return;
	}
	status = zdoInit();
	if (status == NEW_NETWORK) {
		show("Start new network");
		status = MT_RPC_SUCCESS;
	} else if (status == RESTORED_NETWORK) {
		show("Restored network");
		status = MT_RPC_SUCCESS;
	} else {
		show("Start failed");
		status = -1;
	}
	show("ZigBee state machine running");
}

void vAppTaskLoop() {
	struct AppMessage xRxedStructure;
	if (xQueueReceive(xQueueViewToBackend, (struct AppMessage*) &xRxedStructure,
			(TickType_t) 10) == pdPASS) {
		switch (xRxedStructure.ucMessageID) {
		case MID_ZB_RESET_COO:
			resetCoo();
			break;
		case MID_ZB_RESET_RTR:
			resetRtr();
			break;
		case MID_ZB_ZBEE_START:
			start();
			break;
		case MID_ZB_ZBEE_SCAN:
			scan();
			break;
		case MID_ZB_ZBEE_LQIREQ: {
			MgmtLqiReqFormat_t lqiReq = { .DstAddr = 0, .StartIndex = 1 };
			memccpy(&lqiReq, xRxedStructure.content, 1,
					sizeof(MgmtLqiReqFormat_t));
			zdoMgmtLqiReq(&lqiReq);
		}
			break;
		}
	}
	rpcWaitMqClientMsg(10);
}

/////////////////////////////////////////////////
void vAppTask(void *pvParameters) {
	cfgRead();
	sysRegisterCallbacks(mtSysCb);
	zdoRegisterCallbacks(mtZdoCb);
	show("System started");
	//znp_if_init();
	znp_if_init();
	znp_cmd_init();
	vTaskDelay(1000);
	uint8_t status = 0;
	status = sysResetReq(&const_hard_rst);
	do {
		vTaskDelay(1000);
		status = sysVersion();
	} while (status != 0);
	show("Event loop started");
	while (1)
		vAppTaskLoop();
}

/********************************************************************
 * START OF ZDO CALL BACK FUNCTIONS
 */

static uint8_t mtZdoStateChangeIndCb(uint8_t newDevState) {

	switch (newDevState) {
	case DEV_HOLD:
		show("Hold");
		break;
	case DEV_INIT:
		show("Init");
		break;
	case DEV_NWK_DISC:
		show("Network Discovering");
		break;
	case DEV_NWK_JOINING:
		show("Network Joining");
		break;
	case DEV_NWK_REJOIN:
		show("Network Rejoining");
		break;
	case DEV_END_DEVICE_UNAUTH:
		show("Network Authenticating");
		break;
	case DEV_END_DEVICE:
		show("Network Joined as End Device");
		break;
	case DEV_ROUTER:
		show("Network Joined as Router");
		break;
	case DEV_COORD_STARTING:
		show("Network Starting");
		break;
	case DEV_ZB_COORD:
		show("Network Started");
		break;
	case DEV_NWK_ORPHAN:
		show("Network Orphaned");
		break;
	default:
		show("Unknown state");
		break;
	}

	devState = (devStates_t) newDevState;

	return SUCCESS;
}

static uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg) {
	if (msg->Status == MT_RPC_SUCCESS) {
		if (msg->StartIndex == 0) {
			show("Parent: %d, Children: %d", msg->SrcAddr,
					msg->NeighborTableEntries);
		}
		if (msg->StartIndex + msg->NeighborLqiListCount
				<= msg->NeighborTableEntries) {
			MgmtLqiReqFormat_t lqiReq;
			lqiReq.DstAddr = msg->SrcAddr;
			lqiReq.StartIndex = msg->StartIndex + msg->NeighborLqiListCount;
			struct AppMessage msg = { .ucMessageID = MID_ZB_ZBEE_LQIREQ };
			memcpy(msg.content, &lqiReq, sizeof(MgmtLqiReqFormat_t));
			xQueueSend(xQueueViewToBackend, (void* ) &msg, (TickType_t ) 0);
		}
		/*
		 uint8_t devType = 0;
		 uint8_t devRelation = 0;
		 uint8_t localNodeCount = nodeCount;

		 show("Device: %d, Children: %d, Type: %d", msg->SrcAddr,
		 msg->NeighborLqiListCount,
		 (msg->SrcAddr == 0 ? DEVICETYPE_COORDINATOR : DEVICETYPE_ROUTER));
		 nodeList[nodeCount].Status = nodeList[nodeCount].NodeAddr =
		 msg->SrcAddr;
		 nodeList[nodeCount].Type = (
		 msg->SrcAddr == 0 ? DEVICETYPE_COORDINATOR : DEVICETYPE_ROUTER);
		 nodeList[localNodeCount].ChildCount = 0;
		 */
		uint32_t i;
		for (i = 0; i < msg->NeighborLqiListCount; i++) {
			show("Index: %d, Found: %d", msg->StartIndex,
					msg->NeighborLqiList[i].NetworkAddress);
			/*
			 devType = msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat & 3;
			 devRelation = ((msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat
			 >> 4) & 7);
			 if (devRelation == 1) {
			 uint8_t cCount = nodeList[localNodeCount].ChildCount;
			 nodeList[localNodeCount].childs[cCount].ChildAddr =
			 msg->NeighborLqiList[i].NetworkAddress;
			 nodeList[localNodeCount].childs[cCount].Type = devType;
			 nodeList[localNodeCount].ChildCount++;
			 if (devType == DEVICETYPE_ROUTER) {
			 MgmtLqiReqFormat_t lqiReq1;
			 lqiReq1.DstAddr = msg->NeighborLqiList[i].NetworkAddress;
			 lqiReq1.StartIndex = 0;
			 struct AppMessage msg = { .ucMessageID = MID_ZB_ZBEE_LQIREQ };
			 memcpy(msg.content, &lqiReq1, sizeof(MgmtLqiReqFormat_t));
			 xQueueSend(xQueueViewToBackend, (void* ) &msg, (TickType_t ) 0);
			 }
			 }
			 */
		}
	}
	return msg->Status;
}

// helper functions for building and sending the NV messages
static uint8_t setNVStartup(uint8_t startupOption) {
	uint8_t status;
	OsalNvWriteFormat_t nvWrite;
	// sending startup option

	nvWrite.Id = ZCD_NV_STARTUP_OPTION;
	nvWrite.Offset = 0;
	nvWrite.Len = 1;
	nvWrite.Value[0] = startupOption;
	status = sysOsalNvWrite(&nvWrite);
	dbg_print(PRINT_LEVEL_INFO, "");

	dbg_print(PRINT_LEVEL_INFO, "NV Write Startup Option cmd sent[%d]..",
			status);

	return status;
}

static uint8_t setNVPanID(uint32_t panId) {
	uint8_t status;
	OsalNvWriteFormat_t nvWrite;

	dbg_print(PRINT_LEVEL_INFO, "");
	dbg_print(PRINT_LEVEL_INFO, "NV Write PAN ID cmd sending..");

	nvWrite.Id = ZCD_NV_PANID;
	nvWrite.Offset = 0;
	nvWrite.Len = 2;
	nvWrite.Value[0] = LO_UINT16(panId);
	nvWrite.Value[1] = HI_UINT16(panId);
	status = sysOsalNvWrite(&nvWrite);
	dbg_print(PRINT_LEVEL_INFO, "");
	dbg_print(PRINT_LEVEL_INFO, "NV Write PAN ID cmd sent...[%d]", status);

	return status;
}

static uint8_t setNVChanList(uint32_t chanList) {
	OsalNvWriteFormat_t nvWrite;
	uint8_t status;
	// setting chanList
	nvWrite.Id = ZCD_NV_CHANLIST;
	nvWrite.Offset = 0;
	nvWrite.Len = 4;
	nvWrite.Value[0] = BREAK_UINT32(chanList, 0);
	nvWrite.Value[1] = BREAK_UINT32(chanList, 1);
	nvWrite.Value[2] = BREAK_UINT32(chanList, 2);
	nvWrite.Value[3] = BREAK_UINT32(chanList, 3);
	status = sysOsalNvWrite(&nvWrite);
	dbg_print(PRINT_LEVEL_INFO, "");
	dbg_print(PRINT_LEVEL_INFO, "NV Write Channel List cmd sent...[%d]",
			status);

	return status;
}

static int32_t registerAf(void) {
	int32_t status = 0;
	RegisterFormat_t reg;

	reg.EndPoint = 1;
	reg.AppProfId = 0x0104;
	reg.AppDeviceId = 0x0100;
	reg.AppDevVer = 1;
	reg.LatencyReq = 0;
	reg.AppNumInClusters = 1;
	reg.AppInClusterList[0] = 0x0006;
	reg.AppNumOutClusters = 0;

	status = afRegister(&reg);
	return status;
}

static uint8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg) {

	if (msg->Status == MT_RPC_SUCCESS) {
		show("\tEndpoint: 0x%02X\n", msg->Endpoint);
		show("\tProfileID: 0x%04X\n", msg->ProfileID);
		show("\tDeviceID: 0x%04X\n", msg->DeviceID);
		show("\tDeviceVersion: 0x%02X\n", msg->DeviceVersion);
		show("\tNumInClusters: %d\n", msg->NumInClusters);
		uint32_t i;
		for (i = 0; i < msg->NumInClusters; i++) {
			show("\t\tInClusterList[%d]: 0x%04X\n", i, msg->InClusterList[i]);
		}
		show("\tNumOutClusters: %d\n", msg->NumOutClusters);
		for (i = 0; i < msg->NumOutClusters; i++) {
			show("\t\tOutClusterList[%d]: 0x%04X\n", i, msg->OutClusterList[i]);
		}
		show("\n");
	} else {
		show("SimpleDescRsp Status: FAIL 0x%02X\n", msg->Status);
	}

	return msg->Status;
}
static uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg) {

	SimpleDescReqFormat_t simReq;
	if (msg->Status == MT_RPC_SUCCESS) {
		simReq.DstAddr = msg->NwkAddr;
		simReq.NwkAddrOfInterest = msg->NwkAddr;
		show("Number of Endpoints: %d\n", msg->ActiveEPCount);
		uint32_t i;
		for (i = 0; i < msg->ActiveEPCount; i++) {
			simReq.Endpoint = msg->ActiveEPList[i];
			zdoSimpleDescReq(&simReq);
		}
	} else {
		show("ActiveEpRsp Status: FAIL 0x%02X\n", msg->Status);
	}

	return msg->Status;
}

static uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg) {

	ActiveEpReqFormat_t actReq;
	actReq.DstAddr = msg->NwkAddr;
	actReq.NwkAddrOfInterest = msg->NwkAddr;
	show("\nNew device joined network.\nNwkAddr: 0x%04X\n", msg->NwkAddr);
	zdoActiveEpReq(&actReq);
	return 0;
}
