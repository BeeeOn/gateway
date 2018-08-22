#include "iqrf/response/DPACoordRemoveNodeResponse.h"

using namespace BeeeOn;

size_t DPACoordRemoveNodeResponse::count() const
{
	return peripheralData()[0];
}
