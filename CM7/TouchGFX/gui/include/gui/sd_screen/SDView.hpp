#ifndef SDVIEW_HPP
#define SDVIEW_HPP

#include <gui_generated/sd_screen/SDViewBase.hpp>
#include <gui/sd_screen/SDPresenter.hpp>

class SDView : public SDViewBase
{
public:
    SDView();
    virtual ~SDView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void btnSaveClick();
    virtual void btnLoadClick();
protected:
};

#endif // SDVIEW_HPP
