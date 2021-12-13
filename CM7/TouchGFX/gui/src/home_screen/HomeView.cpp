#include <gui/home_screen/HomeView.hpp>

#include <appTask.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <memory.h>

HomeView::HomeView() {
}

void HomeView::displayMessage(char *msg) {
	uint16_t length = strlen(msg) + 1;
	Unicode::UnicodeChar *start = textAreaBuffer;
	Unicode::UnicodeChar *begin = start + length;
	Unicode::UnicodeChar *stop = start + TEXTAREA_SIZE - 1;
	while (stop >= begin) {
		Unicode::UnicodeChar *pointer = stop - length;
		*stop = *pointer;
		stop = stop - 1;
	}
	Unicode::strncpy(textAreaBuffer, msg, length - 1);
	Unicode::strncpy(textAreaBuffer + length - 1, "\n", 1);
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
