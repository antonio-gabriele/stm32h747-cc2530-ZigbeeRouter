#include <gui/home_screen/HomeView.hpp>

#include <appTask.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <memory.h>

extern QueueHandle_t xQueueViewToBackend;

HomeView::HomeView() {
}

void HomeView::show(char *msg) {
	Unicode::snprintf(textAreaBuffer, TEXTAREA_SIZE, msg);
	//Unicode::fromUTF8((uint8_t*)msg, textAreaBuffer, TEXTAREA_SIZE);
	this->textArea.invalidate();
}

void HomeView::btnPairClick() {
	AppMessage msg;
	msg.ucMessageID = MID_ZB_PAIR;
	memset(msg.content, 0, sizeof(msg.content));
	xQueueSend(xQueueViewToBackend, (void* ) &msg, (TickType_t ) 0);
}

void HomeView::btnStartClick() {
	AppMessage msg;
	msg.ucMessageID = MID_ZB_START;
	memset(msg.content, 0, sizeof(msg.content));
	xQueueSend(xQueueViewToBackend, (void* ) &msg, (TickType_t ) 0);
}

void HomeView::setupScreen() {
	HomeViewBase::setupScreen();
}

void HomeView::tearDownScreen() {
	HomeViewBase::tearDownScreen();
}
