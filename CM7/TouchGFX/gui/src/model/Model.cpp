#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#include <appTask.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>

extern QueueHandle_t xQueueBackendToView;

Model::Model() : modelListener(0)
{

}

void Model::tick()
{
	struct AppMessage xRxedStructure;
	if (xQueueReceive(xQueueBackendToView,
			(struct AppMessage*) &xRxedStructure, (TickType_t) 10)
			== pdPASS) {
		this->modelListener->receive(&xRxedStructure);
	}

}
