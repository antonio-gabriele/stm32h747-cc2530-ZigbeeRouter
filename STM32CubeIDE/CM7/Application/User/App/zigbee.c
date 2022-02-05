#include <zigbee.h>

#include "cmsis_os.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <timers.h>
#include <queue.h>
#include <system.h>
/********************************************************************************/
extern Configuration_t sys_cfg;
extern TimerHandle_t xTimer;
extern QueueHandle_t xQueue;
/********************************************************************************/
devStates_t zb_device_state = DEV_HOLD;
mtUtilCb_t mtUtilCb = { .pfnUtilGetDeviceInfoCb_t = pfnUtilGetDeviceInfoCb };
mtAfCb_t mtAfCb = { .pfnAfDataConfirm = NULL, //
		.pfnAfDataReqeuestSrsp = NULL, //
		.pfnAfDataRetrieveSrsp = NULL, //
		.pfnAfIncomingMsg = zb_af_incoming_msg, //
		.pfnAfIncomingMsgExt = NULL, //
		.pfnAfReflectError = NULL, //
		.pfnAfRegisterSrsp = NULL //
		};
mtSysCb_t mtSysCb = { NULL, NULL, NULL, mtSysResetIndCb, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
mtZdoCb_t mtZdoCb = { NULL,       // MT_ZDO_NWK_ADDR_RSP
		mtZdoIeeeAddrRspCb,      // MT_ZDO_IEEE_ADDR_RSP
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
		zb_zdo_bind,          // MT_ZDO_BIND_RSP
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
/********************************************************************************/
uint8_t af_counter = 0;
extern uint8_t machineState;
extern utilGetDeviceInfoFormat_t system;
/********************************************************************************/
uint8_t pfnUtilGetDeviceInfoCb(utilGetDeviceInfoFormat_t *msg) {
	if (machineState == 2) {
		machineState = 0;
		memcpy(&system, msg, sizeof(utilGetDeviceInfoFormat_t));
		appPrintf("I'm: %04X.%llu", msg->short_addr, msg->ieee_addr);
	}
	return MT_RPC_SUCCESS;
}

uint8_t mtSysResetIndCb(ResetIndFormat_t *msg) {
	printf("--------------------------------\n");
	if (machineState == 1) {
		machineState = 0;
		appPrintf("System started");
		Fake_t req;
		RUN(appStartStack, req)
	}
	return MT_RPC_SUCCESS;
}

uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg) {
	xTimerReset(xTimer, 0);
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	Node_t *node = zbFindNodeByAddress(msg->NwkAddr);
	if (node->ActiveEndpointRetry == ZB_OK) {
		zbRepairNode(node, true);
		return MT_RPC_SUCCESS;
	}
	printf("AEND: %04X -> %d", msg->NwkAddr, msg->Status);
	node->ActiveEndpointRetry = ZB_OK;
	node->EndpointCount = msg->ActiveEPCount;
	uint32_t i;
	for (i = 0; i < msg->ActiveEPCount; i++) {
		uint8_t endpoint = msg->ActiveEPList[i];
		node->Endpoints[i].Endpoint = endpoint;
		node->Endpoints[i].SimpleDescriptorRetry = ZB_RE;
		node->Endpoints[i].C06Exists = ZB_KO;
		printf(" %d", endpoint);
	}
	printf("\n");
	zbRepairNode(node, true);
	return msg->Status;
}

uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg) {
	xTimerReset(xTimer, 0);
	printf("Joined: 0x%04X\n", msg->NwkAddr);
	Node_t *node = zbFindNodeByAddress(msg->NwkAddr);
	if (node == NULL) {
		node = zbFindNodeByIEEE(msg->IEEEAddr);
		if (node == NULL) {
			node = &sys_cfg.Nodes[sys_cfg.NodesCount];
			sys_cfg.NodesCount++;
			node->LqiRetry = ZB_RE;
			node->NameRetry = ZB_RE;
			node->ActiveEndpointRetry = ZB_RE;
			node->ManufacturerName[0] = 0;
			node->ModelIdentifier[0] = 0;
		}
		node->IEEERetry = ZB_OK;
		node->Address = msg->NwkAddr;
		node->IEEE = msg->IEEEAddr;
	}
	zbRepairNode(node, true);
	return MT_RPC_SUCCESS;
}
uint8_t mtZdoIeeeAddrRspCb(IeeeAddrRspFormat_t *msg) {
	xTimerReset(xTimer, 0);
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	Node_t *node = zbFindNodeByAddress(msg->NwkAddr);
	printf("IEEE: %04X -> %llu \n", msg->NwkAddr, msg->IEEEAddr);
	node->IEEE = msg->IEEEAddr;
	node->IEEERetry = ZB_OK;
	zbRepairNode(node, true);
	return MT_RPC_SUCCESS;
}

uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg) {
	xTimerReset(xTimer, 0);
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	if (msg->StartIndex + msg->NeighborLqiListCount < msg->NeighborTableEntries) {
		MgmtLqiReqFormat_t req = { .DstAddr = msg->SrcAddr, .StartIndex = msg->StartIndex + msg->NeighborLqiListCount };
		RUN(zdoMgmtLqiReq, req)
	}
	if (msg->StartIndex + msg->NeighborLqiListCount == msg->NeighborTableEntries) {
		Node_t *node = zbFindNodeByAddress(msg->SrcAddr);
		node->LqiRetry = ZB_OK;
		zbRepairNode(node, true);
	}
	uint32_t iNeighbor = 0;
	uint16_t address;
	for (iNeighbor = 0; iNeighbor < msg->NeighborLqiListCount; iNeighbor++) {
		address = msg->NeighborLqiList[iNeighbor].NetworkAddress;
		Node_t *node = zbFindNodeByAddress(address);
		if (node == NULL) {
			printf("RLQI: %04X <- Welcome\n", address);
			node = &sys_cfg.Nodes[sys_cfg.NodesCount];
			sys_cfg.NodesCount++;
			node->LqiRetry = ZB_RE;
			node->ActiveEndpointRetry = ZB_RE;
			node->IEEERetry = ZB_RE;
			node->NameRetry = ZB_RE;
			node->Address = address;
			node->EndpointCount = 0;
			node->IEEE = 0;
			node->ManufacturerName[0] = 0;
			node->ModelIdentifier[0] = 0;
			zbRepairNode(node, true);
		}
	}
	return msg->Status;
}

uint8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg) {
	xTimerReset(xTimer, 0);
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	Node_t *node = zbFindNodeByAddress(msg->NwkAddr);
	if (node == NULL) {
		return MT_RPC_SUCCESS;
	}
	Endpoint_t *endpoint = zbFindEndpoint(node, msg->Endpoint);
	if (endpoint == NULL || endpoint->SimpleDescriptorRetry == ZB_OK) {
		return MT_RPC_SUCCESS;
	}
	if (msg->Endpoint == 0) {
		endpoint->SimpleDescriptorRetry = ZB_KO;
		return msg->Status;
	}
	endpoint->SimpleDescriptorRetry = ZB_OK;
	uint32_t i;
	for (i = 0; i < msg->NumInClusters; i++) {
		if(msg->InClusterList[i] == 0x06){
			printf("SDES.06: %04X.%d -> %d\n", msg->NwkAddr, msg->Endpoint, msg->Status);
			endpoint->C06Exists = ZB_OK;
			endpoint->C06BindRetry = ZB_RE;
			endpoint->C06ValueRetry = ZB_RE;
		}
	}
	zbRepairNode(node, true);
	return msg->Status;
}

uint8_t mtZdoStateChangeIndCb(uint8_t newDevState) {
	switch (newDevState) {
	case DEV_HOLD:
		appPrintf("Hold");
		break;
	case DEV_INIT:
		appPrintf("Init");
		break;
	case DEV_NWK_DISC:
		appPrintf("Network Discovering");
		break;
	case DEV_NWK_JOINING:
		appPrintf("Network Joining");
		break;
	case DEV_NWK_REJOIN:
		appPrintf("Network Rejoining");
		break;
	case DEV_END_DEVICE_UNAUTH:
		appPrintf("Network Authenticating");
		break;
	case DEV_END_DEVICE:
		appPrintf("Network Joined as End Device");
		break;
	case DEV_ROUTER:
		appPrintf("Network Joined as Router");
		break;
	case DEV_COORD_STARTING:
		appPrintf("Network Starting");
		break;
	case DEV_ZB_COORD:
		appPrintf("Network Started");
		break;
	case DEV_NWK_ORPHAN:
		appPrintf("Network Orphaned");
		break;
	default:
		break;
	}
	zb_device_state = (devStates_t) newDevState;
	return SUCCESS;
}

uint8_t zb_zdo_bind(BindRspFormat_t *msg) {
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	printf("BIND: %04X\n", msg->SrcAddr);
	return MT_RPC_SUCCESS;
}

uint8_t zbCount(Summary_t *summary) {
	uint8_t iNode;
	for (iNode = 0; iNode < sys_cfg.NodesCount; iNode++) {
		Node_t *node = &sys_cfg.Nodes[iNode];
		summary->nNodes++;
		if (node->ActiveEndpointRetry == ZB_OK) {
			summary->nNodesAEOk++;
		}
		if (node->IEEERetry == ZB_OK) {
			summary->nNodesIEEEOk++;
		}
		if (node->LqiRetry == ZB_OK) {
			summary->nNodesLQOk++;
		}
		if (node->NameRetry == ZB_OK) {
			summary->nNodesNameOk++;
		}
		uint8_t iEndpoint;
		summary->nEndpoints += node->EndpointCount;
		for (iEndpoint = 0; iEndpoint < node->EndpointCount; iEndpoint++) {
			Endpoint_t *endpoint = &node->Endpoints[iEndpoint];
			if (endpoint->SimpleDescriptorRetry == ZB_OK) {
				summary->nEndpointsSDOk++;
			}
			if (endpoint->SimpleDescriptorRetry != ZB_OK) {
				break;
			}
			if (endpoint->C06Exists == ZB_OK) {
				if (endpoint->C06BindRetry == ZB_OK) {
					summary->nEndpointsBindOk++;
				}
				if (endpoint->C06ValueRetry == ZB_OK) {
					summary->nEndpointsValueOk++;
				}
			}
		}
	}
	return MT_RPC_SUCCESS;
}

uint8_t zbStartScan(Fake_t *f) {
	{
		Node_t *node = zbFindNodeByAddress(system.short_addr);
		if (node == NULL) {
			node = &sys_cfg.Nodes[sys_cfg.NodesCount];
			sys_cfg.NodesCount++;
			node->LqiRetry = ZB_RE;
			node->NameRetry = ZB_OK;
			node->ActiveEndpointRetry = ZB_RE;
			strcpy(node->ManufacturerName, "Antonio Gabriele");
			strcpy(node->ModelIdentifier, "Fire.v1.0");
			node->IEEERetry = ZB_OK;
			node->Address = system.short_addr;
			node->IEEE = system.ieee_addr;
		}
		{
			MgmtLqiReqFormat_t req = { .DstAddr = system.short_addr, .StartIndex = 0 };
			RUN(zdoMgmtLqiReq, req)
		}
	}
	{
		Node_t *node = zbFindNodeByAddress(0);
		if (node == NULL) {
			node = &sys_cfg.Nodes[sys_cfg.NodesCount];
			sys_cfg.NodesCount++;
			node->LqiRetry = ZB_RE;
			node->NameRetry = ZB_RE;
			node->ActiveEndpointRetry = ZB_RE;
			node->IEEERetry = ZB_RE;
			node->Address = 0;
		}
		{
			MgmtLqiReqFormat_t req = { .DstAddr = 0, .StartIndex = 0 };
			RUN(zdoMgmtLqiReq, req)
		}
	}
	return MT_RPC_SUCCESS;
}

uint8_t zbRepairNode(Node_t *node, bool reset) {
	uint8_t iEndpoint;
	if (reset) {
		if (node->LqiRetry == ZB_KO)
			node->LqiRetry = ZB_RE;
		if (node->IEEERetry == ZB_KO)
			node->IEEERetry = ZB_RE;
		if (node->ActiveEndpointRetry == ZB_KO)
			node->ActiveEndpointRetry = ZB_RE;
		if (node->NameRetry == ZB_KO)
			node->NameRetry = ZB_RE;
		for (iEndpoint = 0; iEndpoint < node->EndpointCount; iEndpoint++) {
			Endpoint_t *endpoint = &node->Endpoints[iEndpoint];
			if (endpoint->SimpleDescriptorRetry == ZB_KO)
				endpoint->SimpleDescriptorRetry = ZB_RE;
			if (endpoint->C06ValueRetry == ZB_KO)
				endpoint->C06ValueRetry = ZB_RE;
			if (endpoint->C06BindRetry == ZB_KO)
				endpoint->C06BindRetry = ZB_RE;
		}
	}
	if (node->LqiRetry != ZB_OK && node->LqiRetry != ZB_KO) {
		node->LqiRetry++;
		MgmtLqiReqFormat_t req = { .DstAddr = node->Address, .StartIndex = 0 };
		RUN(zdoMgmtLqiReq, req)
	}
	if (node->IEEERetry != ZB_OK && node->IEEERetry != ZB_KO) {
		node->IEEERetry++;
		IeeeAddrReqFormat_t req = { .ShortAddr = node->Address, .ReqType = 0, .StartIndex = 0 };
		RUN(zdoIeeeAddrReq, req)
	}
	if (node->IEEERetry != ZB_OK) {
		return MT_RPC_SUCCESS;
	}
	if (node->ActiveEndpointRetry != ZB_OK && node->ActiveEndpointRetry != ZB_KO) {
		node->ActiveEndpointRetry++;
		ActiveEpReqFormat_t req = { .DstAddr = node->Address, .NwkAddrOfInterest = node->Address };
		RUN(zdoActiveEpReq, req)
	}
	if (node->NameRetry != ZB_OK && node->NameRetry != ZB_KO) {
		node->NameRetry++;
		DataRequestFormat_t req = { .ClusterID = 0, .DstAddr = node->Address, .DstEndpoint = 1, .SrcEndpoint = 1, .Len = 7, .Options = 0, .Radius = 0, .TransID = 1 };
		req.Data[0] = 0; //ZCL->1
		req.Data[1] = (af_counter++) % 255;
		req.Data[2] = 0; //CmdId
		req.Data[3] = 0x05;
		req.Data[4] = 0x00;
		req.Data[5] = 0x04;
		req.Data[6] = 0x00;
		RUN(afDataRequest, req)
	}
	if (node->ActiveEndpointRetry != ZB_OK) {
		return MT_RPC_SUCCESS;
	}
	for (iEndpoint = 0; iEndpoint < node->EndpointCount; iEndpoint++) {
		Endpoint_t *endpoint = &node->Endpoints[iEndpoint];
		if (endpoint->SimpleDescriptorRetry != ZB_OK && endpoint->SimpleDescriptorRetry != ZB_KO) {
			endpoint->SimpleDescriptorRetry++;
			SimpleDescReqFormat_t req = { .DstAddr = node->Address, .NwkAddrOfInterest = node->Address, .Endpoint = endpoint->Endpoint };
			RUN(zdoSimpleDescReq, req)
		}
		if(endpoint->SimpleDescriptorRetry != ZB_OK){
			break;
		}
		if(endpoint->C06Exists == ZB_OK){
			if (endpoint->C06ValueRetry != ZB_OK && endpoint->C06ValueRetry != ZB_KO) {
				endpoint->C06ValueRetry++;
				DataRequestFormat_t req = { .ClusterID = 0x06, .DstAddr = node->Address, .DstEndpoint = endpoint->Endpoint, .SrcEndpoint = 1, .Len = 5, .Options = 0, .Radius = 0, .TransID = 1 };
				req.Data[0] = 0x00;
				req.Data[1] = (af_counter++) % 255;
				req.Data[2] = 0x00;
				req.Data[3] = 0x00;
				req.Data[4] = 0x00;
				RUN(afDataRequest, req)
			}
			if (endpoint->C06BindRetry != ZB_OK && endpoint->C06BindRetry != ZB_KO) {
				endpoint->C06BindRetry++;
				BindReqFormat_t req = { .ClusterID = 0x06, .DstAddr = system.short_addr, .DstEndpoint = 1, .SrcEndpoint = endpoint->Endpoint, .SrcAddress = node->Address };
				RUN(zdoBindReq, req)
			}
		}
	}
	return MT_RPC_SUCCESS;
}

static uint8_t retry = 0;

uint8_t zbRepair(Fake_t *f) {
	uint8_t iNode;
	bool reset = ((retry++) % 30) == 0;
	for (iNode = 0; iNode < sys_cfg.NodesCount; iNode++) {
		Node_t *node = &sys_cfg.Nodes[iNode];
		zbRepairNode(node, reset);
	}
	return MT_RPC_SUCCESS;
}

uint8_t zb_af_incoming_msg_cmd01_c00(IncomingMsgFormat_t *msg) {
	Node_t *node = zbFindNodeByAddress(msg->SrcAddr);
	uint8_t j = 3;
	while (j + 2 < msg->Len) {
		uint16_t currentAttributeId = msg->Data[j++];
		currentAttributeId |= msg->Data[j++] << 8;
		uint8_t success = msg->Data[j++];
		if (success == 0) {
			uint8_t zigbeeType = msg->Data[j++];
			if (zigbeeType == 66) {
				uint8_t strlen = msg->Data[j++];
				uint8_t *str = &(msg->Data[j]);
				if (currentAttributeId == 5) {
					memcpy(node->ModelIdentifier, str, strlen);
					node->ModelIdentifier[strlen] = 0;
				} else {
					memcpy(node->ManufacturerName, str, strlen);
					node->ModelIdentifier[strlen] = 0;
				}
				j += strlen;
				node->NameRetry = ZB_OK;
			}
		}
	}
	return MT_RPC_SUCCESS;;
}

uint8_t zb_af_incoming_msg_cmd01_c06(IncomingMsgFormat_t *msg) {
	Node_t *node = zbFindNodeByAddress(msg->SrcAddr);
	uint8_t j = 3;
	while (j + 2 < msg->Len) {
		uint16_t currentAttributeId = msg->Data[j++];
		currentAttributeId |= msg->Data[j++] << 8;
		uint8_t success = msg->Data[j++];
		if (success == 0) {
			uint8_t zigbeeType = msg->Data[j++];
			if (zigbeeType == 16) {
				Endpoint_t *endpoint = zbFindEndpoint(node, msg->SrcEndpoint);
				endpoint->C06ValueRetry = ZB_OK;
				endpoint->C06Value = msg->Data[j++];
			}
		}
	}
	return MT_RPC_SUCCESS;
}

uint8_t zb_af_incoming_msg_cmd01(IncomingMsgFormat_t *msg) {
	switch (msg->ClusterId) {
	case 0:
		zb_af_incoming_msg_cmd01_c00(msg);
		break;
	case 6:
		zb_af_incoming_msg_cmd01_c06(msg);
		break;
	}
	return MT_RPC_SUCCESS;
}
uint8_t zb_af_incoming_msg(IncomingMsgFormat_t *msg) {
	xTimerReset(xTimer, 0);
	switch (msg->Data[2]) {
	case 0x01:
		zb_af_incoming_msg_cmd01(msg);
		break;
	}
	return MT_RPC_SUCCESS;
}

Endpoint_t* zbFindEndpoint(Node_t *node, uint8_t endpoint) {
	uint8_t i1;
	for (i1 = 0; i1 < node->EndpointCount; ++i1) {
		if (node->Endpoints[i1].Endpoint == endpoint) {
			return &node->Endpoints[i1];
		}
	}
	return NULL;
}

Node_t* zbFindNodeByAddress(uint16_t address) {
	uint8_t i1;
	for (i1 = 0; i1 < sys_cfg.NodesCount; ++i1) {
		if (sys_cfg.Nodes[i1].Address == address) {
			return &sys_cfg.Nodes[i1];
		}
	}
	return NULL;
}

Node_t* zbFindNodeByIEEE(uint64_t ieee) {
	uint8_t i1;
	for (i1 = 0; i1 < sys_cfg.NodesCount; ++i1) {
		if (sys_cfg.Nodes[i1].IEEE == ieee) {
			return &sys_cfg.Nodes[i1];
		}
	}
	return NULL;
}

void zbInit() {
	utilRegisterCallbacks(mtUtilCb);
	sysRegisterCallbacks(mtSysCb);
	zdoRegisterCallbacks(mtZdoCb);
	afRegisterCallbacks(mtAfCb);
}
