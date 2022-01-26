#include <gui/home_screen/HomeView.hpp>
#include <gui/home_screen/HomePresenter.hpp>

#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>
#include <string.h>

extern QueueHandle_t xQueueBackendToView;
extern QueueHandle_t xQueueViewToBackend;

HomePresenter::HomePresenter(HomeView &v) :
		view(v) {
}

void HomePresenter::tick() {
	struct AppMessage xRxedStructure;
	if (xQueueReceive(xQueueBackendToView, (struct AppMessage*) &xRxedStructure, (TickType_t) 10) == pdPASS) {
		switch (xRxedStructure.ucMessageID) {
		case MID_VW_LOG:
			this->view.displayMessage(xRxedStructure.content);
			break;
		}
	}
}

void HomePresenter::scan() {
	void * req = NULL;
	RUN(app_scanner, req)
}

void HomePresenter::start() {
	void * req = NULL;
	RUN(app_start_stack, req)
}

void HomePresenter::reset(Fake_t devType) {
	RUN(app_reset, devType)
}

void HomePresenter::activate() {
	this->model->bind(this);
}

void HomePresenter::deactivate() {

}
