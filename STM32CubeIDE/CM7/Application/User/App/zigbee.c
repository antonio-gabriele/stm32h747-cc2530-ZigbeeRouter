#include <zigbee.h>

#include "cmsis_os.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <timers.h>
#include <queue.h>
#include <system.h>
/********************************************************************************/
extern Configuration_t sys_cfg;
extern TimerHandle_t xTimer;
extern QueueHandle_t xQueueViewToBackend;
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
mtSysCb_t mtSysCb = { NULL, NULL, NULL, zb_sys_reset, zb_sys_version, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
mtZdoCb_t mtZdoCb = { NULL,       // MT_ZDO_NWK_ADDR_RSP
		zb_zdo_ieee_address,      // MT_ZDO_IEEE_ADDR_RSP
		NULL,      // MT_ZDO_NODE_DESC_RSP
		NULL,     // MT_ZDO_POWER_DESC_RSP
		zb_zdo_simple_descriptor,    // MT_ZDO_SIMPLE_DESC_RSP
		zb_zdo_active_endpoint,      // MT_ZDO_ACTIVE_EP_RSP
		NULL,     // MT_ZDO_MATCH_DESC_RSP
		NULL,   // MT_ZDO_COMPLEX_DESC_RSP
		NULL,      // MT_ZDO_USER_DESC_RSP
		NULL,     // MT_ZDO_USER_DESC_CONF
		NULL,    // MT_ZDO_SERVER_DISC_RSP
		NULL, // MT_ZDO_END_DEVICE_BIND_RSP
		zb_zdo_bind,          // MT_ZDO_BIND_RSP
		NULL,        // MT_ZDO_UNBIND_RSP
		NULL,   // MT_ZDO_MGMT_NWK_DISC_RSP
		zb_zdo_mgmt_remote_lqi,       // MT_ZDO_MGMT_LQI_RSP
		NULL,       // MT_ZDO_MGMT_RTG_RSP
		NULL,      // MT_ZDO_MGMT_BIND_RSP
		NULL,     // MT_ZDO_MGMT_LEAVE_RSP
		NULL,     // MT_ZDO_MGMT_DIRECT_JOIN_RSP
		NULL,     // MT_ZDO_MGMT_PERMIT_JOIN_RSP
		zb_zdo_state_changed,   // MT_ZDO_STATE_CHANGE_IND
		zb_zdo_end_device_announce,   // MT_ZDO_END_DEVICE_ANNCE_IND
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
extern utilGetDeviceInfoFormat_t system;
/********************************************************************************/
uint8_t pfnUtilGetDeviceInfoCb(utilGetDeviceInfoFormat_t *msg) {
	memcpy(&system, msg, sizeof(utilGetDeviceInfoFormat_t));
	app_show("I'm: %04X.%llu", msg->short_addr, msg->ieee_addr);
	return MT_RPC_SUCCESS;
}

uint8_t zb_sys_reset(ResetIndFormat_t *msg) {
	printf("Reset ZNP Version: %d.%d.%d\n", msg->MajorRel, msg->MinorRel, msg->HwRev);
	return 0;
}

uint8_t zb_sys_version(VersionSrspFormat_t *msg) {
	printf("Request ZNP Version: %d.%d.%d\n", msg->MajorRel, msg->MinorRel, msg->Product);
	return 0;
}

uint8_t zb_zdo_state_changed(uint8_t newDevState) {

	switch (newDevState) {
	case DEV_HOLD:
		app_show("Hold");
		break;
	case DEV_INIT:
		app_show("Init");
		break;
	case DEV_NWK_DISC:
		app_show("Network Discovering");
		break;
	case DEV_NWK_JOINING:
		app_show("Network Joining");
		break;
	case DEV_NWK_REJOIN:
		app_show("Network Rejoining");
		break;
	case DEV_END_DEVICE_UNAUTH:
		app_show("Network Authenticating");
		break;
	case DEV_END_DEVICE:
		app_show("Network Joined as End Device");
		break;
	case DEV_ROUTER:
		app_show("Network Joined as Router");
		break;
	case DEV_COORD_STARTING:
		app_show("Network Starting");
		break;
	case DEV_ZB_COORD:
		app_show("Network Started");
		break;
	case DEV_NWK_ORPHAN:
		app_show("Network Orphaned");
		break;
	default:
		//app_show("Unknown state");
		break;
	}

	zb_device_state = (devStates_t) newDevState;

	return SUCCESS;
}

uint8_t zb_zdo_explore1(Summary_t *summary) {
	uint8_t iNode;
	for (iNode = 0; iNode < sys_cfg.NodesCount; iNode++) {
		Node_t *node = &sys_cfg.Nodes[iNode];
		zb_zdo_explore(node, summary);
	}
	return MT_RPC_SUCCESS;
}

uint8_t zb_zdo_ieee_address(IeeeAddrRspFormat_t *msg) {
	xTimerReset(xTimer, 0);
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	printf("IEEE: %04X -> %llu \n", msg->NwkAddr, msg->IEEEAddr);
	Node_t *node = zb_find_node_by_address(msg->NwkAddr);
	node->IEEE = msg->IEEEAddr;
	node->IEEERetry = ZB_OK;
	Summary_t summary;
	zb_zdo_explore(node, &summary);
	return MT_RPC_SUCCESS;
}

uint8_t zb_zdo_bind(BindRspFormat_t *msg) {
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	printf("BIND: %04X\n", msg->SrcAddr);
	return MT_RPC_SUCCESS;
}

uint8_t zb_zdo_explore(Node_t *node, Summary_t *summary) {
	summary->nNodes++;
	if (node->ActiveEndpointRetry != ZB_OK && node->ActiveEndpointRetry != ZB_KO) {
		node->ActiveEndpointRetry++;
		ActiveEpReqFormat_t req = { .DstAddr = node->Address, .NwkAddrOfInterest = node->Address };
		RUN(zdoActiveEpReq, req)
		return MT_RPC_SUCCESS;
	} else {
		summary->nNodesAEOk++;
	}
	if (node->IEEE == 0 && node->IEEERetry != ZB_OK && node->IEEERetry != ZB_KO) {
		node->IEEERetry++;
		IeeeAddrReqFormat_t req = { .ShortAddr = node->Address, .ReqType = 0, .StartIndex = 0 };
		RUN(zdoIeeeAddrReq, req)
		return MT_RPC_SUCCESS;
	}
	summary->nNodesIEEEOk++;
	if (node->LqiRetry != ZB_OK && node->LqiRetry != ZB_KO) {
		node->LqiRetry++;
		MgmtLqiReqFormat_t req = { .DstAddr = node->Address, .StartIndex = 0 };
		RUN(zdoMgmtLqiReq, req)
		return MT_RPC_SUCCESS;
	} else {
		summary->nNodesLQOk++;
	}
	if ((node->ManufacturerName[0] == 0x00 || node->ModelIdentifier[0] == 0x00) && node->NameRetry != ZB_OK && node->NameRetry != ZB_KO) {
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
		return MT_RPC_SUCCESS;
	} else {
		summary->nNodesNameOk++;
	}

	uint8_t iEndpoint;
	summary->nEndpoints += node->EndpointCount;
	for (iEndpoint = 0; iEndpoint < node->EndpointCount; iEndpoint++) {
		Endpoint_t *endpoint = &node->Endpoints[iEndpoint];
		if (endpoint->SimpleDescriptorRetry != ZB_OK && endpoint->SimpleDescriptorRetry != ZB_KO) {
			endpoint->SimpleDescriptorRetry++;
			SimpleDescReqFormat_t req = { .DstAddr = node->Address, .NwkAddrOfInterest = node->Address, .Endpoint = endpoint->Endpoint };
			RUN(zdoSimpleDescReq, req)
		} else {
			summary->nEndpointsSDOk++;
		}
		/*
		uint8_t iCluster;
		for (iCluster = 0; iCluster < endpoint->InClusterCount; iCluster++) {
			Cluster_t *cluster = &(endpoint->InClusters[iCluster]);
			if (cluster->B0 > 0) {
				BindReqFormat_t req = { .ClusterID = 0x06, .DstAddr = system.short_addr, .DstEndpoint = 1, .SrcEndpoint = endpoint->Endpoint, .SrcAddress = node->Address };
				RUN(zdoBindReq, req)
			} else {
				summary->nEndpointsBindOk++;
			}
		}
		*/
	}

	return MT_RPC_SUCCESS;
}

uint8_t zb_zdo_end_device_announce(EndDeviceAnnceIndFormat_t *msg) {
	printf("Joined: 0x%04X\n", msg->NwkAddr);
	Node_t *node = zb_find_node_by_address(msg->NwkAddr);
	if (node == NULL) {
		node = zb_find_node_by_ieee(msg->IEEEAddr);
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
	Summary_t summary;
	zb_zdo_explore(node, &summary);
	return MT_RPC_SUCCESS;
}

uint8_t zb_zdo_mgmt_remote_lqi(MgmtLqiRspFormat_t *msg) {
	xTimerReset(xTimer, 0);
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	if (msg->StartIndex + msg->NeighborLqiListCount < msg->NeighborTableEntries) {
		MgmtLqiReqFormat_t req = { .DstAddr = msg->SrcAddr, .StartIndex = msg->StartIndex + msg->NeighborLqiListCount };
		RUN(zdoMgmtLqiReq, req)
	}
	if (msg->StartIndex + msg->NeighborLqiListCount == msg->NeighborTableEntries) {
		Node_t *node1 = zb_find_node_by_address(msg->SrcAddr);
		node1->LqiRetry = ZB_OK;
	}
	uint32_t iNeighbor = 0;
	uint16_t address;
	for (iNeighbor = 0; iNeighbor < msg->NeighborLqiListCount; iNeighbor++) {
		address = msg->NeighborLqiList[iNeighbor].NetworkAddress;
		Node_t *node = zb_find_node_by_address(address);
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
			Summary_t summary;
			zb_zdo_explore(node, &summary);
		}
	}
	return msg->Status;
}

uint8_t zb_zdo_simple_descriptor(SimpleDescRspFormat_t *msg) {
	xTimerReset(xTimer, 0);
	Node_t *node = zb_find_node_by_address(msg->NwkAddr);
	if (node == NULL) {
		return MT_RPC_SUCCESS;
	}
	Endpoint_t *endpoint = zb_find_endpoint(node, msg->Endpoint);
	if (endpoint == NULL) {
		return MT_RPC_SUCCESS;
	}
	if (msg->Endpoint == 0) {
		endpoint->SimpleDescriptorRetry = ZB_KO;
		return msg->Status;
	}
	printf("SDES: %04X.%d -> %d", msg->SrcAddr, msg->Endpoint, msg->Status);
	endpoint->SimpleDescriptorRetry = ZB_OK;
	endpoint->InClusterCount = msg->NumInClusters;
	endpoint->OutClusterCount = msg->NumOutClusters;
	uint32_t i;
	for (i = 0; i < msg->NumInClusters; i++) {
		printf(" I%d", msg->InClusterList[i]);
		endpoint->InClusters[i].Cluster = msg->InClusterList[i];
		if (msg->InClusterList[i] == 0x06) {
			endpoint->InClusters[i].B0 = ZB_KO;
			DataRequestFormat_t req = { .ClusterID = 6, .DstAddr = msg->NwkAddr, .DstEndpoint = msg->Endpoint, .SrcEndpoint = 1, .Len = 5, .Options = 0, .Radius = 0, .TransID = 1 };
			req.Data[0] = 0x00;
			req.Data[1] = (af_counter++) % 255;
			req.Data[2] = 0x00;
			req.Data[3] = 0x00;
			req.Data[4] = 0x00;
			RUN(afDataRequest, req)
		}
	}
	for (i = 0; i < msg->NumOutClusters; i++) {
		printf(" O%d", msg->OutClusterList[i]);
		endpoint->OutClusters[i].Cluster = msg->OutClusterList[i];
	}
	printf("\n");
	return msg->Status;
}

uint8_t zb_zdo_active_endpoint(ActiveEpRspFormat_t *msg) {
	xTimerReset(xTimer, 0);
	printf("AEND: %04X -> %d", msg->SrcAddr, msg->Status);
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	Node_t *node = zb_find_node_by_address(msg->SrcAddr);
	node->ActiveEndpointRetry = ZB_OK;
	node->EndpointCount = msg->ActiveEPCount;
	uint32_t i;
	for (i = 0; i < msg->ActiveEPCount; i++) {
		uint8_t endpoint = msg->ActiveEPList[i];
		node->Endpoints[i].Endpoint = endpoint;
		node->Endpoints[i].SimpleDescriptorRetry = ZB_RE;
		printf(" %d", endpoint);
		Summary_t summary;
		zb_zdo_explore(node, &summary);
	}
	printf("\n");
	return msg->Status;
}

uint8_t zb_af_incoming_msg_cmd01_c00(IncomingMsgFormat_t *msg) {
	Node_t *node = zb_find_node_by_address(msg->SrcAddr);
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
	Node_t *node = zb_find_node_by_address(msg->SrcAddr);
	uint8_t j = 3;
	while (j + 2 < msg->Len) {
		uint16_t currentAttributeId = msg->Data[j++];
		currentAttributeId |= msg->Data[j++] << 8;
		uint8_t success = msg->Data[j++];
		if (success == 0) {
			uint8_t zigbeeType = msg->Data[j++];
			if (zigbeeType == 16) {
				Endpoint_t *endpoint = zb_find_endpoint(node, msg->SrcEndpoint);
				Cluster_t *cluster = zb_find_cluster(endpoint, msg->ClusterId);
				cluster->A0 = msg->Data[j++];
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

Cluster_t* zb_find_cluster(Endpoint_t *endpoint, uint16_t cluster) {
	uint8_t i1;
	for (i1 = 0; i1 < endpoint->InClusterCount; ++i1) {
		if (endpoint->InClusters[i1].Cluster == cluster) {
			return &endpoint->InClusters[i1];
		}
	}
	return NULL;
}

Endpoint_t* zb_find_endpoint(Node_t *node, uint8_t endpoint) {
	uint8_t i1;
	for (i1 = 0; i1 < node->EndpointCount; ++i1) {
		if (node->Endpoints[i1].Endpoint == endpoint) {
			return &node->Endpoints[i1];
		}
	}
	return NULL;
}

Node_t* zb_find_node_by_address(uint16_t address) {
	uint8_t i1;
	for (i1 = 0; i1 < sys_cfg.NodesCount; ++i1) {
		if (sys_cfg.Nodes[i1].Address == address) {
			return &sys_cfg.Nodes[i1];
		}
	}
	return NULL;
}

Node_t* zb_find_node_by_ieee(uint64_t ieee) {
	uint8_t i1;
	for (i1 = 0; i1 < sys_cfg.NodesCount; ++i1) {
		if (sys_cfg.Nodes[i1].IEEE == ieee) {
			return &sys_cfg.Nodes[i1];
		}
	}
	return NULL;
}

void zb_init() {
	utilRegisterCallbacks(mtUtilCb);
	sysRegisterCallbacks(mtSysCb);
	zdoRegisterCallbacks(mtZdoCb);
	afRegisterCallbacks(mtAfCb);
}
