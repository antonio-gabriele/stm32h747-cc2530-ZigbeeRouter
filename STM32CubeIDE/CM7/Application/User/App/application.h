#ifndef APPLICATION_USER_CORE_APPTASK_H_
#define APPLICATION_USER_CORE_APPTASK_H_

#include <stdint.h>

#define MAX_NODES 100
#define MAX_ENDPS 8
#define MAX_CLUSR 8

typedef struct {
	uint16_t Cluster;
	
} Cluster_t;

typedef struct {
	uint8_t Endpoint;
	uint8_t SimpleDescriptorCompleted;
	uint8_t InClusterCount;
	Cluster_t InClusters[MAX_CLUSR];
	uint8_t OutClusterCount;
	Cluster_t OutClusters[MAX_CLUSR];
} Endpoint_t;

typedef struct {
	uint8_t LqiCompleted;
	uint8_t ActiveEndpointCompleted;
	uint16_t Address;
	uint64_t IEEE;
	uint8_t Type;
	uint8_t EndpointCount;
	uint8_t ManufacturerName[32];
	uint8_t ModelIdentifier[32];
	Endpoint_t Endpoints[MAX_ENDPS];
} Node_t;

typedef struct {
	uint8_t MagicNumber;
	uint8_t DeviceType;
	uint8_t NodesCount;
	Node_t Nodes[MAX_NODES];
} Configuration_t;

#define MID_ZB_RESET_COO 		0x00
#define MID_ZB_RESET_RTR 		0x01
#define MID_ZB_ZBEE_START 		0x02
#define MID_ZB_ZBEE_SCAN 		0x03

#define MID_ZB_ZBEE_LQIREQ		0x04
#define MID_ZB_ZBEE_ACTEND		0x05
#define MID_ZB_ZBEE_SIMDES		0x06
#define MID_ZB_ZBEE_DATARQ		0x07

#define MID_VW_LOG				0

#define ENQUEUE(ID, STRUCTNAME, OBJECT) struct AppMessage message = { .ucMessageID = ID }; \
		memcpy(message.content, &OBJECT, sizeof(STRUCTNAME)); \
		xQueueSend(xQueueViewToBackend, (void* ) &message, (TickType_t ) 0);

#define DEQUEUE(STRUCTNAME, FN) { STRUCTNAME req; \
			memcpy(&req, xRxedStructure.content, sizeof(STRUCTNAME)); \
			FN(&req); }

struct AppMessage
{
    char ucMessageID;
    char content[140];
};

void app_init();
void app_reset(uint8_t devType);
void app_summary();
uint8_t app_show(const char *fmt, ...);

#endif
