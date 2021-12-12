#include <gui/home_screen/HomeView.hpp>
#include <gui/home_screen/HomePresenter.hpp>

#include <appTask.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>

HomePresenter::HomePresenter(HomeView &v) :
		view(v) {
}

void HomePresenter::receive(struct AppMessage *message) {
	switch (message->ucMessageID) {
	case MID_VW_LOG:
		this->view.show(message->content);
		break;
	}
}


void HomePresenter::activate() {
	this->model->bind(this);
}

void HomePresenter::deactivate() {

}
