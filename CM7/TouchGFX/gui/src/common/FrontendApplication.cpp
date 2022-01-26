#include <gui/common/FrontendApplication.hpp>

static struct Model *model;

FrontendApplication::FrontendApplication(Model &m, FrontendHeap &heap) :
		FrontendApplicationBase(m, heap) {
	model = m;
}
