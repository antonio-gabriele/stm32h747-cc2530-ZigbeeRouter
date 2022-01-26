#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

Model::Model() :
		modelListener(0) {
}

void Model::tick() {
	this->modelListener->tick();
}


void Model::displayMessage(char * message) {
	this->modelListener->displayMessage(message);
}
