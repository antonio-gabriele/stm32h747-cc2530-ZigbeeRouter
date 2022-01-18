#include <gui/home_screen/HomeView.hpp>
#include <gui/home_screen/HomePresenter.hpp>

#include <application.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <queue.h>

extern QueueHandle_t xQueueBackendToView;

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

void HomePresenter::beToUi(struct AppMessage *message) {
}

void HomePresenter::uiToBe(struct AppMessage *message) {
	this->model->uiToBe(message);
}

void HomePresenter::activate() {
	this->model->bind(this);
}

void HomePresenter::deactivate() {

}
