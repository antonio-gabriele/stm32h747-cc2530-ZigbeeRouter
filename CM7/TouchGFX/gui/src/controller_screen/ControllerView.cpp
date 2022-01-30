#include <gui/controller_screen/ControllerView.hpp>
#include <application.h>

extern Configuration_t sys_cfg;

Tuple1_t Cluster[100];
uint8_t ClustersCount;

ControllerView::ControllerView() {

}

void ControllerView::deviceScrollListUpdateItem(Device &item, int16_t itemIndex) {
	item.bind(&(Cluster[itemIndex]));
}

void ControllerView::setupScreen() {
	ClustersCount = 0;
	for (int iNode = 0; iNode < sys_cfg.NodesCount; iNode++) {
		Node_t *node = &(sys_cfg.Nodes[iNode]);
		for (int iEndpoint = 0; iEndpoint < node->EndpointCount; iEndpoint++) {
			Endpoint_t *endpoint = &(node->Endpoints[iEndpoint]);
			for (int iCluster = 0; iCluster < endpoint->InClusterCount; iCluster++) {
				Cluster_t *cluster = &(endpoint->InClusters[iCluster]);
				if (cluster->Cluster == 6) {
					Cluster[ClustersCount].Node = node;
					Cluster[ClustersCount].Endpoint = endpoint;
					Cluster[ClustersCount++].Cluster = cluster;
				}
			}
		}
	}
	this->deviceScrollList.setNumberOfItems(ClustersCount);
	ControllerViewBase::setupScreen();
}

void ControllerView::tearDownScreen() {
	ControllerViewBase::tearDownScreen();
}
