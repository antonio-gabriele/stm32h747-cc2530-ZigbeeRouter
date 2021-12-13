#include <gui/home_screen/HomeView.hpp>

#include <appTask.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <memory.h>

HomeView::HomeView() {
}

void HomeView::displayMessage(char *msg) {
	uint16_t length = strlen(msg);
	Unicode::UnicodeChar *stop = textAreaBuffer + TEXTAREA_SIZE;
	while (--stop > (textAreaBuffer + length)) {
		*stop = *(stop - length - 1);
	}
    Unicode::strncpy(textAreaBuffer, msg, length);
	Unicode::strncpy(textAreaBuffer + length, "\n", 1);
	this->textArea.invalidate();
}

void HomeView::btnPairClick() {
	this->presenter->uiPair();
}

void HomeView::btnStartClick() {
	this->presenter->uiStart();
}

void HomeView::setupScreen() {
	HomeViewBase::setupScreen();
}

void HomeView::tearDownScreen() {
	HomeViewBase::tearDownScreen();
}
