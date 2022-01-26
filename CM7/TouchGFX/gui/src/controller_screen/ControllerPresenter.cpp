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

void ControllerPresenter::displayMessage(char * message){
	this->view.displayMessage(message);
}

void ControllerPresenter::activate()
{
}

void ControllerPresenter::deactivate()
{
}

void ControllerPresenter::tick() {
}
