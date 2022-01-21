#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <gui_generated/containers/DeviceBase.hpp>
#include <application.h>

class Device : public DeviceBase
{
public:
    Device();
    virtual ~Device() {}
    virtual void bind(Node_t *);
    virtual void initialize();
    virtual void btnOnOffClick();
private:
    Node_t * node;
protected:
};

#endif // DEVICE_HPP
