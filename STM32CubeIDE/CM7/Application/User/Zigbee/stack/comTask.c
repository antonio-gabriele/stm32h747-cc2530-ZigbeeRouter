#include <comTask.h>
#include "cmsis_os.h"
#include "rpc.h"

void vPollTask(void *pvParameters) {
	// endless loop, handle CC2530 packets
	while (1) {
		rpcWaitMqClientMsg(0xFFFF);
	}
}

void vComTask(void *pvParameters) {
	// init queues
	rpcInitMq();

	// initialize serial port
	rpcOpen();

	// start poll task
	xTaskCreate(vPollTask, "POLL", 1024, NULL, 5, NULL);

	// loop
	while (1) {
		// keep procesing packets
		rpcProcess();

		// give other tasks time to run
		vTaskDelay(1);
	}
}
