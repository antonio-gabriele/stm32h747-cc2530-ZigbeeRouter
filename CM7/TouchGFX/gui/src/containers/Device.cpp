#include <gui/containers/Device.hpp>

#include <memory.h>
#include <cstdio>
#include <application.h>
#include <mtAf.h>
#include <FreeRTOS.h>
#include <queue.h>

extern QueueHandle_t xQueue;
extern uint8_t af_counter;

Device::Device() {

}

void Device::refresh(){
	char caption[64] = { 0 };
	char *mn = (char*) tuple->Node->ManufacturerName;
	//char *mi = (char*) tuple->Node->ModelIdentifier;
	uint8_t activeState = tuple->Endpoint->C06Value;
	sprintf(caption, "%s %04X.%d", mn, tuple->Node->Address, tuple->Endpoint->Endpoint);
	//uint8_t length = strlen(caption) > BTNONOFFCAPTION_SIZE ? BTNONOFFCAPTION_SIZE : strlen(caption);
	Unicode::strncpy(this->btnOnOffCaptionBuffer, caption, BTNONOFFCAPTION_SIZE);
	this->btnOnOff.forceState(activeState);
	this->btnOnOff.invalidate();
	this->btnOnOffCaption.invalidate();
}

void Device::btnOnOffClick() {
	uint8_t activeState = this->btnOnOff.getState();
	DataRequestFormat_t req;
	req.ClusterID = 6;
	req.DstAddr = this->tuple->Node->Address;
	req.DstEndpoint = this->tuple->Endpoint->Endpoint;
	req.SrcEndpoint = 1;
	req.Len = 3;
	req.Options = 0;
	req.Radius = 0;
	req.TransID = 1;
	req.Data[0] = 1;
	req.Data[1] = (af_counter++)%255;
	req.Data[2] = activeState; //CmdId
	RUNNOW(afDataRequest, req);
}

void Device::bind(Tuple1_t *tuple) {
	this->tuple = tuple;
	this->ieee = tuple->Node->IEEE;
	this->endpoint = tuple->Endpoint->Endpoint;
	this->refresh();
}

void Device::initialize() {
	DeviceBase::initialize();
}
