#include <gui/home_screen/HomeView.hpp>
#include <gui/home_screen/HomePresenter.hpp>

#include <appTask.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>

HomePresenter::HomePresenter(HomeView &v) :
		view(v) {
}

void HomePresenter::beToUi(struct AppMessage *message) {
	switch (message->ucMessageID) {
	case MID_VW_LOG:
		this->view.displayMessage(message->content);
		break;
	}
}

void HomePresenter::uiPair() {
	AppMessage msg = { 0 };
	msg.ucMessageID = MID_ZB_PAIR;
	this->model->uiToBe(&msg);
}

void HomePresenter::uiStart() {
	AppMessage msg = { 0 };
	msg.ucMessageID = MID_ZB_START;
	this->model->uiToBe(&msg);
}

void HomePresenter::activate() {
	this->model->bind(this);
}

void HomePresenter::deactivate() {

}
