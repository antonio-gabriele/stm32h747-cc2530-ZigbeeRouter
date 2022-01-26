#ifndef CONTROLLERPRESENTER_HPP
#define CONTROLLERPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ControllerView;

class ControllerPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ControllerPresenter(ControllerView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();
    virtual void tick();
    virtual ~ControllerPresenter() {};

private:
    ControllerPresenter();

    ControllerView& view;
};

#endif // CONTROLLERPRESENTER_HPP
