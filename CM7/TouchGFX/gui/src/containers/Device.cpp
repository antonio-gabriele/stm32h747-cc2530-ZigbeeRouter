#include <gui/containers/Device.hpp>

#include <memory.h>
#include <cstdio>
#include <application.h>
#include <mtAf.h>
#include <FreeRTOS.h>
#include <queue.h>

extern QueueHandle_t xQueueViewToBackend;

Device::Device() {

}

void Device::btnOnOffClick() {
	DataRequestFormat_t req;
	req.ClusterID = 6;
	req.DstAddr = this->node->Address;
	req.DstEndpoint = 1;
	req.SrcEndpoint = 1;
	req.Len = 3;
	req.Options = 0;
	req.Radius = 0;
	req.TransID = 1;
	req.Data[0] = 1;
	req.Data[1] = 0xFF;
	req.Data[2] = 2; //CmdId
	RUN(afDataRequest, req);
}

void Device::bind(Node_t *node) {
	this->node = node;
	char caption[64] = { 0 };
	char *mn = (char*) node->ManufacturerName;
	printf(mn);
	char *mi = (char*) node->ModelIdentifier;
	printf(mi);
	sprintf(caption, "%s %04X", mi, node->Address);
	uint8_t length = strlen(caption) > BTNONOFFCAPTION_SIZE ? BTNONOFFCAPTION_SIZE : strlen(caption);
	Unicode::strncpy(this->btnOnOffCaptionBuffer, caption, length);
	this->btnOnOffCaption.invalidate();
}

void Device::initialize() {
	DeviceBase::initialize();
}
