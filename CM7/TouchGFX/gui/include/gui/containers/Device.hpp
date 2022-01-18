#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <gui_generated/containers/DeviceBase.hpp>

class Device : public DeviceBase
{
public:
    Device();
    virtual ~Device() {}
    virtual void setCaption(char *);
    virtual void initialize();
protected:
};

#endif // DEVICE_HPP
