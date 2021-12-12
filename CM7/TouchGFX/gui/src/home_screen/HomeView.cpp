#include <gui/home_screen/HomeView.hpp>

#include <appTask.h>
#include <FreeRTOS.h>
#include <queue.h>

extern QueueHandle_t xQueueViewToBackend;

HomeView::HomeView() {

}

void HomeView::btnPairClick() {
	AppMessage msg;
	msg.ucMessageID = 0;
	msg.content = NULL;
	xQueueSend(xQueueViewToBackend, (void*) &msg,
			(TickType_t) 0);
}

void HomeView::btnStartClick() {
	AppMessage msg;
	msg.ucMessageID = 1;
	msg.content = NULL;
	xQueueSend(xQueueViewToBackend, (void*) &msg,
			(TickType_t) 0);
}

void HomeView::setupScreen() {
	HomeViewBase::setupScreen();
}

void HomeView::tearDownScreen() {
	HomeViewBase::tearDownScreen();
}
