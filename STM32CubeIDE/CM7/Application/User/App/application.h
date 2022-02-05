#ifndef APPLICATION_USER_CORE_APPTASK_H_
#define APPLICATION_USER_CORE_APPTASK_H_

#include <stdint.h>

#define MAX_NODES 100
#define MAX_ENDPS 8
#define MAX_CLUSR 8
#define TEXTAREA_SIZE1 8192

typedef struct {
	uint16_t Cluster;
	uint8_t A0;
	uint8_t B0;
} Cluster_t;

typedef struct {
	uint8_t Endpoint;
	uint8_t SimpleDescriptorRetry;
	uint8_t InClusterCount;
	Cluster_t InClusters[MAX_CLUSR];
	uint8_t OutClusterCount;
	Cluster_t OutClusters[MAX_CLUSR];
} Endpoint_t;

typedef struct {
	uint8_t LqiRetry;
	uint8_t ActiveEndpointRetry;
	uint16_t Address;
	uint64_t IEEE;
	uint8_t IEEERetry;
	uint8_t Type;
	uint8_t EndpointCount;
	uint8_t NameRetry;
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

typedef struct {
	uint8_t nNodes;
	uint8_t nNodesAEOk;
	uint8_t nNodesIEEEOk;
	uint8_t nNodesLQOk;
	uint8_t nNodesNameOk;
	uint8_t nEndpoints;
	uint8_t nEndpointsSDOk;
	uint8_t nEndpointsBindOk;
	Endpoint_t *E06[64];
} Summary_t;

typedef union {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
} Fake_t;

typedef struct {
	Cluster_t * Cluster;
	Endpoint_t * Endpoint;
	Node_t * Node;
} Tuple1_t;

#define MID_VW_LOG				0

#define RUN(FN,PAR)	\
	struct AppMessage message = { .fn = (void (*)(void*)) (&FN) }; \
	memcpy(message.params, &PAR, sizeof(PAR)); \
	xQueueSendToFront(xQueueViewToBackend, (void* ) &message, (TickType_t ) portMAX_DELAY);

#define RUNNOW(FN,PAR)	\
	struct AppMessage message = { .fn = (void (*)(void*)) (&FN) }; \
	memcpy(message.params, &PAR, sizeof(PAR)); \
	xQueueSendToBack(xQueueViewToBackend, (void* ) &message, (TickType_t ) portMAX_DELAY);

struct AppMessage {
	void (*fn)(void*);
	char params[140];
};

uint8_t app_scanner(void *none);
uint8_t appStartStack(void *none);
uint8_t app_init(void *none);
uint8_t app_reset(Fake_t *devType);
uint8_t app_summary(void *none);
uint8_t app_show(const char *fmt, ...);

#endif
