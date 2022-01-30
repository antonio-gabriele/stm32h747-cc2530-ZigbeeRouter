#include <gui/home_screen/HomeView.hpp>

#include <application.h>
#include <memory.h>

static struct HomeView *homeView;

static const uint16_t TEXTAREA_SIZE = 8192;
static touchgfx::Unicode::UnicodeChar textAreaBuffer1[TEXTAREA_SIZE] = { 0 };

extern "C" void call_C_displayMessage(char *message) {
	if (homeView)
		homeView->displayMessage(message);
}

HomeView::HomeView() {
	flg1 = false;
}

void HomeView::displayMessage(char *msg) {
	uint16_t length = strlen(msg);
	Unicode::UnicodeChar *stop = textAreaBuffer1 + TEXTAREA_SIZE;
	while (--stop > (textAreaBuffer1 + length)) {
		*stop = *(stop - length - 1);
	}
	Unicode::strncpy(textAreaBuffer1, msg, length);
	Unicode::strncpy(textAreaBuffer1 + length, "\n", 1);
	Unicode::strncpy(textAreaBuffer, textAreaBuffer1, TEXTAREA_SIZE);
	flg1 = true;
}

void HomeView::btnScanClick() {
	this->presenter->scan();
}

void HomeView::btnStartClick() {
	this->presenter->start();
}

void HomeView::btnResetCooClick() {
	Fake_t devType = { .u8 = 0 };
	this->presenter->reset(devType);
}

void HomeView::btnResetRtrClick() {
	Fake_t devType = { .u8 = 1 };
	this->presenter->reset(devType);
}

void HomeView::setupScreen() {
	homeView = this;
	HomeViewBase::setupScreen();
	Unicode::strncpy(textAreaBuffer, textAreaBuffer1, TEXTAREA_SIZE);
	this->textArea.invalidate();
}

void HomeView::tearDownScreen() {
	homeView = 0;
	HomeViewBase::tearDownScreen();
}

void HomeView::tick() {
	if (flg1) {
		flg1 = false;
		this->textArea.invalidate();
	}
}
