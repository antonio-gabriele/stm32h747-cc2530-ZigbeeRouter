#include <gui/sd_screen/SDView.hpp>

SDView::SDView()
{

}

void SDView::setupScreen()
{
    SDViewBase::setupScreen();
}

void SDView::tearDownScreen()
{
    SDViewBase::tearDownScreen();
}

void SDView::btnSaveClick(){
	this->presenter->save();
}

void SDView::btnLoadClick(){
	this->presenter->load();
}
