#include <gui/controller_screen/ControllerView.hpp>
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

extern Configuration_t sys_cfg;
extern utilGetDeviceInfoFormat_t system;
static Tuple1_t Item[64];
static uint8_t ItemsCount;
extern QueueHandle_t xQueueUI;

extern "C" void call_C_refreshNodeEndpoint(Node_t *node, Endpoint_t *endpoint) {
	if (node == NULL || endpoint == NULL) {
		Tuple2_t tuple = { .IEEE = node->IEEE, .Endpoint = endpoint->Endpoint };
		xQueueSendToFront(xQueueUI, (void* ) &tuple, (TickType_t ) 10);
	} else {
		Tuple2_t tuple = { 0 };
		xQueueSendToFront(xQueueUI, (void* ) &tuple, (TickType_t ) 10);
	}
}

void ControllerView::tick() {
	Tuple2_t xRxed;
	if (xQueueReceive(xQueueUI, (Tuple2_t*) &xRxed, (TickType_t) 1) == pdPASS) {
		if (xRxed.IEEE > 0 && xRxed.Endpoint > 0) {
			for (int i = 0; i < deviceScrollListListItems.getNumberOfDrawables(); i++) {
				Device *device = &deviceScrollListListItems[i];
				if (device->ieee == xRxed.IEEE && device->endpoint == xRxed.Endpoint) {
					device->refresh();
					return;
				}
			}
		}
		this->setupScreen();
	}
}

ControllerView::ControllerView() {

}

void ControllerView::deviceScrollListUpdateItem(Device &item, int16_t itemIndex) {
	item.bind(&(Item[itemIndex]));
}

void ControllerView::setupScreen() {
	ItemsCount = 0;
	for (int iNode = 0; iNode < sys_cfg.NodesCount; iNode++) {
		Node_t *node = &(sys_cfg.Nodes[iNode]);
		for (int iEndpoint = 0; iEndpoint < node->EndpointCount; iEndpoint++) {
			Endpoint_t *endpoint = &(node->Endpoints[iEndpoint]);
			if (endpoint->C06Exists == ZB1_OK) {
				Item[ItemsCount].Node = node;
				Item[ItemsCount].Endpoint = endpoint;
				ItemsCount++;
			}
		}
	}
	this->deviceScrollList.setNumberOfItems(ItemsCount);
	ControllerViewBase::setupScreen();
}

void ControllerView::tearDownScreen() {
	ItemsCount = 0;
	ControllerViewBase::tearDownScreen();
}
