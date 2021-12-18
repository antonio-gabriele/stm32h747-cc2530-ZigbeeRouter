#include <gui/home_screen/HomeView.hpp>

#include <appTask.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <memory.h>

HomeView::HomeView() {
}

void HomeView::displayMessage(char *msg) {
	uint16_t length = strlen(msg);
	Unicode::UnicodeChar *stop = textAreaBuffer + TEXTAREA_SIZE;
	while (--stop > (textAreaBuffer + length)) {
		*stop = *(stop - length - 1);
	}
    Unicode::strncpy(textAreaBuffer, msg, length);
	Unicode::strncpy(textAreaBuffer + length, "\n", 1);
	this->textArea.invalidate();
}

void HomeView::btnScanClick() {
	AppMessage msg = { .ucMessageID = MID_ZB_ZBEE_SCAN };
	this->presenter->uiToBe(&msg);
}

void HomeView::btnStartClick() {
	AppMessage msg = { .ucMessageID = MID_ZB_ZBEE_START };
	this->presenter->uiToBe(&msg);
}

void HomeView::btnResetCooClick() {
	AppMessage msg = { .ucMessageID = MID_ZB_RESET_COO };
	this->presenter->uiToBe(&msg);
}

void HomeView::btnResetRtrClick() {
	AppMessage msg = { .ucMessageID = MID_ZB_RESET_RTR };
	this->presenter->uiToBe(&msg);
}

void HomeView::setupScreen() {
	HomeViewBase::setupScreen();
}

void HomeView::tearDownScreen() {
	HomeViewBase::tearDownScreen();
}
