#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#include <application.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>

extern QueueHandle_t xQueueViewToBackend;

Model::Model() :
		modelListener(0) {
}

void Model::uiToBe(struct AppMessage *message) {
	xQueueSend(xQueueViewToBackend, (void* ) message, (TickType_t ) 0);
}

void Model::tick() {
	this->modelListener->tick();
	/*
	struct AppMessage xRxedStructure;
	if (xQueueReceive(xQueueBackendToView, (struct AppMessage*) &xRxedStructure,
			(TickType_t) 10) == pdPASS) {
		this->modelListener->beToUi(&xRxedStructure);
	}
	*/
}
