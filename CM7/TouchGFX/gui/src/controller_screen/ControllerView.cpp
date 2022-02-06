#include <gui/controller_screen/ControllerView.hpp>
#include <application.h>
#include <zigbee.h>

extern Configuration_t sys_cfg;
extern utilGetDeviceInfoFormat_t system;
static Tuple1_t Item[64];
static uint8_t ItemsCount;

ControllerView::ControllerView() {

}

void ControllerView::deviceScrollListUpdateItem(Device &item, int16_t itemIndex) {
	item.bind(&(Item[itemIndex]));
}

void ControllerView::setupScreen() {
	ItemsCount = 0;
	for (int iNode = 0; iNode < sys_cfg.NodesCount; iNode++) {
		Node_t *node = &(sys_cfg.Nodes[iNode]);
		if (node->Address == 0 || node->Address == system.short_addr) {
			break;
		}
		for (int iEndpoint = 0; iEndpoint < node->EndpointCount; iEndpoint++) {
			Endpoint_t *endpoint = &(node->Endpoints[iEndpoint]);
			if (endpoint->C06Exists == ZB_OK) {
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
	ControllerViewBase::tearDownScreen();
}
