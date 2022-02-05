#include <gui/home_screen/HomeView.hpp>
#include <gui/home_screen/HomePresenter.hpp>

#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>
#include <string.h>

extern QueueHandle_t xQueueViewToBackend;

HomePresenter::HomePresenter(HomeView &v) :
		view(v) {
}

void HomePresenter::tick() {
	this->view.tick();
}

void HomePresenter::scan() {
	void * req = NULL;
	RUN(app_scanner, req)
}

void HomePresenter::start() {
	void * req = NULL;
	RUN(appStartStack, req)
}

void HomePresenter::reset(Fake_t devType) {
	RUN(app_reset, devType)
}

void HomePresenter::activate() {
	this->model->bind(this);
}

void HomePresenter::deactivate() {
}
