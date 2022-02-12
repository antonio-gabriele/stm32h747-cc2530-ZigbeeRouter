#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <gui_generated/containers/DeviceBase.hpp>
#include <application.h>

class Device : public DeviceBase
{
public:
    Device();
    virtual ~Device() {}
    virtual void bind(Tuple1_t*);
    virtual void initialize();
    virtual void btnOnOffClick();
    virtual void sliderValueConfirmed(int value);
    virtual void refresh();
    uint64_t ieee;
    uint8_t endpoint;
private:
    Tuple1_t * tuple;
protected:
};

#endif // DEVICE_HPP
