#include <gui/containers/Device.hpp>

#include <memory.h>

Device::Device() {

}

void Device::setCaption(char * caption) {
	Unicode::strncpy(this->btnOnOffCaptionBuffer, caption, strlen(caption));
}

void Device::initialize() {
	DeviceBase::initialize();
}
