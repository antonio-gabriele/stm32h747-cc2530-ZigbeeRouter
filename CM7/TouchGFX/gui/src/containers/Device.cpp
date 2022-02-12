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

void Device::refresh() {
	{
		char caption[64] = { 0 };
		char *mn = (char*) tuple->Node->ManufacturerName;
		sprintf(caption, "%s %04X.%d", mn, tuple->Node->Address, tuple->Endpoint->Endpoint);
		Unicode::strncpy(this->btnOnOffCaptionBuffer, caption, BTNONOFFCAPTION_SIZE);
		this->btnOnOffCaption.invalidate();
	}
	/**/
	{
		this->btnOnOff.setVisible(tuple->Endpoint->C06Exists);
		bool activeState = tuple->Endpoint->C06Value;
		this->btnOnOff.forceState(activeState);
		this->btnOnOff.invalidate();
	}
	/**/
	{
		this->slider.setVisible(tuple->Endpoint->C08Exists);
		uint8_t activeState = tuple->Endpoint->C08Value;
		this->slider.setValue(activeState);
		this->slider.invalidate();
	}
}

void Device::sliderValueConfirmed(int value){
	DataRequestFormat_t req;
	req.ClusterID = 8;
	req.DstAddr = this->tuple->Node->Address;
	req.DstEndpoint = this->tuple->Endpoint->Endpoint;
	req.SrcEndpoint = 1;
	req.Len = 8;
	req.Options = 0;
	req.Radius = 0;
	req.TransID = 1;
	req.Data[0] = 0x1; //FrameControl
	req.Data[1] = (af_counter++) % 255; //Sequence
	req.Data[2] = 0x0; //CmdId
	req.Data[3] = value; //CmdPayload
	req.Data[4] = 0x0;
	req.Data[5] = 0x0;
	req.Data[6] = 0x0;
	req.Data[7] = 0x0;
	RUNNOW(afDataRequest, req);
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
	req.Data[0] = 1; //FrameControl
	req.Data[1] = (af_counter++) % 255; //Sequence
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
