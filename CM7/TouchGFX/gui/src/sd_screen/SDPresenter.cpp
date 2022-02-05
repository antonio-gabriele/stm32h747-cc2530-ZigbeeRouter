#include <gui/sd_screen/SDView.hpp>
#include <gui/sd_screen/SDPresenter.hpp>

#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>
#include <string.h>

extern QueueHandle_t xQueue;

SDPresenter::SDPresenter(SDView& v)
    : view(v)
{

}

void SDPresenter::activate()
{

}

void SDPresenter::deactivate()
{

}

void SDPresenter::tick()
{

}

void SDPresenter::save() {
	void * req = NULL;
	RUN(cfgWrite, req)
}

void SDPresenter::load() {
	void * req = NULL;
	RUN(cfgRead, req)
}
