#include <gui/controller_screen/ControllerView.hpp>

extern Configuration_t sys_cfg;

ControllerView::ControllerView() {

}

void ControllerView::deviceScrollListUpdateItem(Device &item, int16_t itemIndex) {
	item.setCaption("Ciao");
}

void ControllerView::devices() {
	this->deviceScrollList.setNumberOfItems(sys_cfg.NodesCount);
}

void ControllerView::setupScreen() {
	ControllerViewBase::setupScreen();
}

void ControllerView::tearDownScreen() {
	ControllerViewBase::tearDownScreen();
}
