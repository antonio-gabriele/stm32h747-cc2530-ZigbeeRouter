#include <gui/controller_screen/ControllerView.hpp>
#include <gui/controller_screen/ControllerPresenter.hpp>

#include <FreeRTOS.h>
#include <queue.h>

extern QueueHandle_t xQueueBackendToView;
extern QueueHandle_t xQueueViewToBackend;

ControllerPresenter::ControllerPresenter(ControllerView& v)
    : view(v)
{

}

void ControllerPresenter::activate()
{

}

void ControllerPresenter::deactivate()
{

}

void ControllerPresenter::tick() {
	struct AppMessage xRxedStructure;
	if (xQueueReceive(xQueueBackendToView, (struct AppMessage*) &xRxedStructure, (TickType_t) 10) == pdPASS) {
		switch (xRxedStructure.ucMessageID) {
		case MID_VW_LOG:
			//this->view.displayMessage(xRxedStructure.content);
			break;
		}
	}

}

void ControllerPresenter::uiToBe(struct AppMessage *message) {
	xQueueSend(xQueueViewToBackend, (void* ) message, (TickType_t ) 0);
}
