#ifndef SDPRESENTER_HPP
#define SDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

extern "C" {
#include <application.h>
#include <system.h>
}

using namespace touchgfx;

class SDView;

class SDPresenter: public touchgfx::Presenter, public ModelListener {
public:
	SDPresenter(SDView &v);

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

	virtual ~SDPresenter() {
	}
	;

	virtual void tick();
	virtual void save();
	virtual void load();
private:
	SDPresenter();

	SDView &view;
};

#endif // SDPRESENTER_HPP
