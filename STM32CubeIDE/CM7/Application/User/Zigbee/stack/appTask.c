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

extern QueueHandle_t xQueueBackendToView;

#define MAX_CHILDREN 10
#define MAX_NODE_LIST 10

//ZDO Callbacks
static uint8_t mtZdoStateChangeIndCb(uint8_t newDevState);
static uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg);

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
typedef enum {
	ZB_DANFOSS_ALLY,
} airmon_zigbee_device_t;

typedef struct {
	uint8_t is_initialized;
	uint32_t last_poll_time;
} zigbee_device_info_t;

typedef struct {
	char manufact_name[32];
	char device_name[32];
	airmon_zigbee_device_t type;
	uint16_t adr_short;
	uint64_t adr_ieee;

	zigbee_device_info_t info; // volatile, should not be saved
} zigbee_device_t;

zigbee_device_t* zigbee_device_get_free_handle(void) {
	static zigbee_device_t device = { 0 };
	return &device;
}

extern QueueHandle_t xQueueViewToBackend;

// SYS callbacks
static mtSysCb_t mtSysCb = {
		NULL,
		NULL,
		NULL,
		mtSysResetIndCb,
		mtSysVersionCb,
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
		NULL,    // MT_ZDO_SIMPLE_DESC_RSP
		NULL,      // MT_ZDO_ACTIVE_EP_RSP
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
		NULL,   // MT_ZDO_END_DEVICE_ANNCE_IND
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

void zigbee_device_save(zigbee_device_t *dev) {

}

void task_maintain_devices(void *param) {

	while (1) {

	}
}

void task_register_device(void *param) {
	// parse parameter
	uint16_t *adr_p = (uint16_t*) param;
	uint16_t adr = *adr_p;

	// feedback
	show("[0x%04x] -> Device init started", adr);

	// get free device handle
	zigbee_device_t *dev = zigbee_device_get_free_handle();

	// sanity check
	if (dev == NULL) {
		// feedback
		show("[0x%04x] -> No more free handles", adr);

		// done, stop the task
		goto end_task;
	}

	// update device address
	dev->adr_short = adr;

	// check if the device already exists
	if (!znp_if_dev_exists(dev->adr_short)) {
		// feedback
		show("[0x%04x] -> Device not registered", dev->adr_short);

		// done, stop the task
		goto end_task;
	}

	// refresh device info
	if (znp_cmd_dev_refresh_info(dev->adr_short) != 0) {
		// feedback
		show("[0x%04x] -> Device couldn't be refreshed", dev->adr_short);

		// done, stop the task
		goto end_task;
	}

	// register device info
	if (znp_cmd_dev_register(dev->adr_short) != 0) {
		// feedback
		show("[0x%04x] -> Device couldn't be registered", dev->adr_short);

		// done, stop the task
		goto end_task;
	}

	// read ieee address
	if (znp_cmd_dev_get_ieee(dev->adr_short, &dev->adr_ieee) != 0) {
		// feedback
		show("[0x%04x] -> Device IEEE couldn't be read", dev->adr_short);

		// done, stop the task
		goto end_task;
	}

	// read manufacturer name cluster
	zcl_cluster_record_t wr;
	if (znp_cmd_cluster_in_read(dev->adr_short, 0x0000, 0x0004, &wr) != 0) {
		// feedback
		show("[0x%04x] -> Device couldn't be read", dev->adr_short);

		// done, stop the task
		goto end_task;
	}

	// check received type
	if (wr.type != ZCL_CHARACTER_STRING) {
		// feedback
		show("[0x%04x] -> Device response type not ok", dev->adr_short);

		// done, stop the task
		goto end_task;
	}

	// save the name
	memcpy(dev->manufact_name, wr.data_arr, MIN(32, wr.data_arr_len));

	// read device name cluster
	if (znp_cmd_cluster_in_read(dev->adr_short, 0x0000, 0x0005, &wr) != 0) {
		// feedback
		show("[0x%04x] -> Device couldn't be read", dev->adr_short);

		// done, stop the task
		goto end_task;
	}

	// check received type
	if (wr.type != ZCL_CHARACTER_STRING) {
		// feedback
		show("[0x%04x] -> Device response type not ok", dev->adr_short);

		// done, stop the task
		goto end_task;
	}

	// save the name
	memcpy(dev->device_name, wr.data_arr, MIN(32, wr.data_arr_len));

	// print device info
	uint32_t top = dev->adr_ieee >> 32;
	uint32_t bot = dev->adr_ieee & 0xffffffff;
	show("[0x%04x] -> IEEE Address: 0x%08x%08x", dev->adr_short, top, bot);
	show("[0x%04x] -> Manufacturer name: %s", dev->adr_short,
			dev->manufact_name);
	show("[0x%04x] -> Device name: %s", dev->adr_short, dev->device_name);

	// save the device
	zigbee_device_save(dev);

// stop the task
	end_task: vTaskDelete(NULL);
}

void device_active_response(uint16_t adr) {
	xTaskCreate(task_register_device, "dev-reg", 1024, (void*) &adr, 6, NULL);
	vTaskDelay(10);
}

void rcpWaitPeriod(uint32_t period) {
	uint32_t waittime = period;
	uint32_t start = xTaskGetTickCount();
	while (rpcWaitMqClientMsg(waittime) == 0) {
		uint32_t passed_time = start - xTaskGetTickCount();
		waittime -= passed_time;
		start = xTaskGetTickCount();
	}
}

void znp_nvm_reset(void) {
	ResetReqFormat_t rst;
	OsalNvWriteFormat_t req;

	// Clear complete memory of CC2530
	req.Id = 0x0003;
	req.Offset = 0x00;
	req.Len = 0x01;
	req.Value[0] = 0x03;
	sysOsalNvWrite(&req);

	// hard reset
	rst.Type = 0x00;
	sysResetReq(&rst);
}

// init coordinator
// taken from https://sunmaysky.blogspot.com/2017/02/use-ztool-z-stack-30-znp-to-set-up.html
int znp_init_coordinator(uint8_t enable_commissioning) {
	OsalNvWriteFormat_t req;
	setChannelFormat_t chn;
	startCommissioningFormat_t strt;
	ResetReqFormat_t rst;

	// wait a second
	vTaskDelay(1000);
	show("1 ----------------------");

	// soft reset
	rst.Type = 0x01;
	sysResetReq(&rst);

	vTaskDelay(4000);
	show("2 ----------------------");

	// Write ZCD_NV_LOGICAL_TYPE to 0 which means coordinator
	req.Id = 0x0087;
	req.Offset = 0x00;
	req.Len = 0x01;
	req.Value[0] = 0x00;
	sysOsalNvWrite(&req);

	vTaskDelay(1000);
	show("3 ----------------------");

	// set primary channel to 13
	chn.primaryChannel = 1;
	chn.channel = CFG_CHANNEL_0x00002000;
	appCfgSetChannel(&chn);

	vTaskDelay(1000);
	show("4 ----------------------");

	// disable secondary channel
	chn.primaryChannel = 0;
	chn.channel = CFG_CHANNEL_NONE;
	appCfgSetChannel(&chn);

	vTaskDelay(1000);
	show("5 ----------------------");

	// start commissioning using network formation
	strt.commissioningMode = CFG_COMM_MODE_NWK_FORMATION;
	appCfgStartCommissioning(&strt);

	vTaskDelay(10000);
	show("6 ----------------------");

	if (enable_commissioning) {
		// get device info
		utilGetDeviceInfo();

		vTaskDelay(1000);
		show("7 ----------------------");

		// Write ZCD_NV_LOGICAL_TYPE to 0 which means coordinator
		req.Id = 0x008F;
		req.Offset = 0x00;
		req.Len = 0x01;
		req.Value[0] = 0x01;
		sysOsalNvWrite(&req);

		vTaskDelay(1000);
		show("8 ----------------------");

		// start commissioning using network steering
		strt.commissioningMode = CFG_COMM_MODE_NWK_STEERING;
		appCfgStartCommissioning(&strt);
	} else {
		// get device info
		utilGetDeviceInfo();
	}

	//
	return 0;
}

void register_clusters(uint16_t addr) {
	vTaskDelay(20000);

	// check registration
	if (!znp_if_dev_exists(addr)) {
		show("-> !!! Device not registered!");
		return;
	}

	show("9 ----------------------");

	// wait for device to be active
	while (1) {
		int ret = znp_cmd_dev_is_active(addr);
		show("znp_if_dev_is_active %d", ret);
		if (ret == 0)
			break;
		vTaskDelay(1000);
	}
	show("10 ----------------------");

	// refresh device info
	show("znp_if_dev_refresh_info %d", znp_cmd_dev_refresh_info(addr));
	vTaskDelay(1000);
	show("11 ----------------------");

	// register device
	show("znp_if_dev_register %d", znp_cmd_dev_register(addr));
	vTaskDelay(1000);
	show("12 ----------------------");

	// read device name cluster
	zcl_cluster_record_t wr;
	show("znp_cmd_cluster_in_read %d",
			znp_cmd_cluster_in_read(addr, 0, 4, &wr));
	show("Type: %d", wr.type);
	show("Str: %s", wr.data_arr);

	// write thermostat to 19 degree
	wr.type = ZCL_SIGNED_16BITS;
	wr.data_i16 = 1900;
	show("znp_cmd_cluster_in_write %d",
			znp_cmd_cluster_in_write(addr, 0x0201, 0x0012, &wr));

	// read thermostat value
	show("znp_cmd_cluster_in_read %d",
			znp_cmd_cluster_in_read(addr, 0x0201, 0x0012, &wr));
	show("Type: %d", wr.type);
	show("Data: %d", wr.data_i16);
}

int32_t startNetwork() {
	char cDevType;
	uint8_t devType;
	int32_t status;
	uint8_t newNwk = 0;
	char sCh[128];
	ResetReqFormat_t resReq;
	resReq.Type = 1;
	sysResetReq(&resReq);
	//flush the rsp
	rpcWaitMqClientMsg(5000);
	registerAf();
	status = setNVDevType(DEVICETYPE_ROUTER);

	if (status != MT_RPC_SUCCESS) {
		show("setNVDevType failed");
		return 0;
	}

	status = zdoInit();
	if (status == NEW_NETWORK) {
		show("zdoInit NEW_NETWORK");
		status = MT_RPC_SUCCESS;
	} else if (status == RESTORED_NETWORK) {
		show("zdoInit RESTORED_NETWORK");
		status = MT_RPC_SUCCESS;
	} else {
		show("zdoInit failed");
		status = -1;
	}

	show("process zdoStatechange callbacks");

	//flush AREQ ZDO State Change messages
	while (status != -1) {
		status = rpcWaitMqClientMsg(5000);
	}
	//set startup option back to keep configuration in case of reset
	status = setNVStartup(0);
	return status;
}

uint8_t show(const char *fmt, ...) {
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

void start() {
	show("Network starting");
	int32_t status;
	status = setNVStartup(0);
	if (status != MT_RPC_SUCCESS) {
		show("Network start failed");
		return;
	}
	startNetwork();
	/*
	 // initialize coordinator
	 znp_init_coordinator(0);

	 // register cluster
	 register_clusters(0x82bc);
	 */
}

void pair() {
	show("Network pairing.");
	int32_t status;
	status = setNVStartup(ZCD_STARTOPT_CLEAR_STATE | ZCD_STARTOPT_CLEAR_CONFIG);
	if (status != MT_RPC_SUCCESS) {
		show("Network start failed");
		return;
	}
	startNetwork();
}

void vAppTaskLoop() {
	struct AppMessage xRxedStructure;
	if (xQueueReceive(xQueueViewToBackend, (struct AppMessage*) &xRxedStructure,
			(TickType_t) 10) == pdPASS) {
		switch (xRxedStructure.ucMessageID) {
		case MID_ZB_START:
			start();
			break;
		case MID_ZB_PAIR:
			pair();
			break;
		}
	}
}

/////////////////////////////////////////////////
void vAppTask(void *pvParameters) {
	show("State Machine Starting.");
	sysRegisterCallbacks(mtSysCb);
	zdoRegisterCallbacks(mtZdoCb);
	show("System Started.");
	znp_if_init();
	znp_if_init();
	znp_cmd_init();
	vTaskDelay(1000);
	uint8_t ret = 0;
	do {
		vTaskDelay(1000);
		ret = sysVersion();
	} while (ret != 0);
	show("Loop Started.");
	while (1)
		vAppTaskLoop();
}

//init ZDO device state
devStates_t devState = DEV_HOLD;
uint8_t gSrcEndPoint = 1;
uint8_t gDstEndPoint = 1;

typedef struct {
	uint16_t ChildAddr;
	uint8_t Type;

} ChildNode_t;

typedef struct {
	uint16_t NodeAddr;
	uint8_t Type;
	uint8_t ChildCount;
	ChildNode_t childs[MAX_CHILDREN];
} Node_t;

Node_t nodeList[MAX_NODE_LIST];
uint8_t nodeCount = 0;

/********************************************************************
 * START OF ZDO CALL BACK FUNCTIONS
 */

static uint8_t mtZdoStateChangeIndCb(uint8_t newDevState) {

	switch (newDevState) {
	case DEV_HOLD:
		show("mtZdoStateChangeIndCb: Initialized - not started automatically");
		break;
	case DEV_INIT:
		show("mtZdoStateChangeIndCb: Initialized - not connected to anything");
		break;
	case DEV_NWK_DISC:
		show("mtZdoStateChangeIndCb: Discovering PAN's to join");
		show("Network Discovering");
		break;
	case DEV_NWK_JOINING:
		show("mtZdoStateChangeIndCb: Joining a PAN");
		show("Network Joining");
		break;
	case DEV_NWK_REJOIN:
		show("Network Rejoining");
		show("mtZdoStateChangeIndCb: ReJoining a PAN, only for end devices");
		break;
	case DEV_END_DEVICE_UNAUTH:
		show("Network Authenticating");
		show("mtZdoStateChangeIndCb: Joined but not yet authenticated by trust center");
		break;
	case DEV_END_DEVICE:
		show("Network Joined");
		show("mtZdoStateChangeIndCb: Started as device after authentication");
		break;
	case DEV_ROUTER:
		show("Network Joined");
		show("mtZdoStateChangeIndCb: Device joined, authenticated and is a router");
		break;
	case DEV_COORD_STARTING:
		show("Network Starting");
		show("mtZdoStateChangeIndCb: Started as Zigbee Coordinator");
		break;
	case DEV_ZB_COORD:
		show("Network Started");
		show("mtZdoStateChangeIndCb: Started as Zigbee Coordinator");
		break;
	case DEV_NWK_ORPHAN:
		show("Network Orphaned");
		show("mtZdoStateChangeIndCb: Device has lost information about its parent");
		break;
	default:
		show("mtZdoStateChangeIndCb: unknown state");
		break;
	}

	devState = (devStates_t) newDevState;

	return SUCCESS;
}

static uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg) {
	uint8_t devType = 0;
	uint8_t devRelation = 0;
	uint8_t localNodeCount = nodeCount;
	MgmtLqiReqFormat_t req;

	if (msg->Status == MT_RPC_SUCCESS) {
		nodeCount++;
		nodeList[localNodeCount].NodeAddr = msg->SrcAddr;
		nodeList[localNodeCount].Type = (
				msg->SrcAddr == 0 ?
				DEVICETYPE_COORDINATOR :
									DEVICETYPE_ROUTER);
		nodeList[localNodeCount].ChildCount = 0;
		uint32_t i;
		for (i = 0; i < msg->NeighborLqiListCount; i++) {
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
					req.DstAddr = msg->NeighborLqiList[i].NetworkAddress;
					req.StartIndex = 0;
					zdoMgmtLqiReq(&req);
				}
			}
		}
	} else {
		//consolePrint("MgmtLqiRsp Status: FAIL 0x%02X", msg->Status);
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

static uint8_t setNVDevType(uint8_t devType) {
	uint8_t status;
	OsalNvWriteFormat_t nvWrite;
	// setting dev type
	nvWrite.Id = ZCD_NV_LOGICAL_TYPE;
	nvWrite.Offset = 0;
	nvWrite.Len = 1;
	nvWrite.Value[0] = devType;
	status = sysOsalNvWrite(&nvWrite);
	show("NV Write Device Type cmd sent... [%d]",
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
