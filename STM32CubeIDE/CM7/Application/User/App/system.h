#ifndef APPLICATION_USER_APP_CONFIGURATION_H_
#define APPLICATION_USER_APP_CONFIGURATION_H_

#define MAX_NODES 100
#define MAX_ENDPS 8
#define MAX_CLUSR 8

#define CFG_ERR 1
#define CFG_OK 0

#include <stdint.h>

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
	uint8_t IEEE[8];
	uint8_t Type;
	uint8_t EndpointCount;
	Endpoint_t Endpoints[MAX_ENDPS];
} Node_t;

typedef struct {
	uint8_t MagicNumber;
    uint8_t DeviceType;
    uint8_t NodesCount;
    Node_t Nodes[MAX_NODES];
} Configuration_t;

uint8_t cfgRead();
uint8_t cfgWrite();

#endif /* APPLICATION_USER_APP_CONFIGURATION_H_ */
