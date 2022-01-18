#ifndef HOMEVIEW_HPP
#define HOMEVIEW_HPP

#include <gui_generated/home_screen/HomeViewBase.hpp>
#include <gui/home_screen/HomePresenter.hpp>

class HomeView : public HomeViewBase
{
public:
    HomeView();
    virtual ~HomeView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void btnScanClick();
    virtual void btnStartClick();
    virtual void btnResetRtrClick();
    virtual void btnResetCooClick();
    virtual void displayMessage(char *);
protected:
};

#endif // HOMEVIEW_HPP
