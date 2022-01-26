#include <gui/controller_screen/ControllerView.hpp>

extern Configuration_t sys_cfg;

ControllerView::ControllerView() {

}

void ControllerView::deviceScrollListUpdateItem(Device &item, int16_t itemIndex) {
	Node_t *node = &sys_cfg.Nodes[itemIndex];
	item.bind(node);
}

void ControllerView::setupScreen() {
	this->deviceScrollList.setNumberOfItems(sys_cfg.NodesCount);
	ControllerViewBase::setupScreen();
}

void ControllerView::tearDownScreen() {
	ControllerViewBase::tearDownScreen();
}

void ControllerView::displayMessage(char* msg) {
}
