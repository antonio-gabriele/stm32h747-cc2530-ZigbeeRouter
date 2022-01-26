#include <gui/home_screen/HomeView.hpp>

#include <application.h>
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

void HomeView::btnScanClick() {
	this->presenter->scan();
}

void HomeView::btnStartClick() {
	this->presenter->start();
}

void HomeView::btnResetCooClick() {
	Fake_t devType = { .u8 = 0};
	this->presenter->reset(devType);
}

void HomeView::btnResetRtrClick() {
	Fake_t devType = { .u8 = 1};
	this->presenter->reset(devType);
}

void HomeView::setupScreen() {
	HomeViewBase::setupScreen();
}

void HomeView::tearDownScreen() {
	HomeViewBase::tearDownScreen();
}
