#include <gui/controller_screen/ControllerView.hpp>
#include <gui/controller_screen/ControllerPresenter.hpp>

#include <FreeRTOS.h>
#include <queue.h>

ControllerPresenter::ControllerPresenter(ControllerView &v) :
		view(v) {

}

void ControllerPresenter::activate() {
	this->model->bind(this);
}

void ControllerPresenter::deactivate() {
}

void ControllerPresenter::tick() {
	this->view.tick();
}
