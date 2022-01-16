#include <zigbee.h>

mtSysCb_t mtSysCb = { NULL, NULL, NULL, mtSysResetIndCb, mtSysVersionCb, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

mtZdoCb_t mtZdoCb = { NULL,       // MT_ZDO_NWK_ADDR_RSP
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

uint8_t mtSysResetIndCb(ResetIndFormat_t *msg) {
	printf("Reset ZNP Version: %d.%d.%d\n", msg->MajorRel, msg->MinorRel, msg->HwRev);
	return 0;
}

uint8_t mtSysVersionCb(VersionSrspFormat_t *msg) {
	printf("Request ZNP Version: %d.%d.%d\n", msg->MajorRel, msg->MinorRel, msg->Product);
	return 0;
}


uint8_t mtZdoStateChangeIndCb(uint8_t newDevState) {

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

uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg) {
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

uint8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg) {
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

uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg) {
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

uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg) {
	printf("Joined: 0x%04X\n", msg->NwkAddr);
	ActiveEpReqFormat_t req = { .DstAddr = msg->NwkAddr, .NwkAddrOfInterest = msg->NwkAddr };
	ENQUEUE(MID_ZB_ZBEE_ACTEND, ActiveEpReqFormat_t, req);
	return 0x00;
}
