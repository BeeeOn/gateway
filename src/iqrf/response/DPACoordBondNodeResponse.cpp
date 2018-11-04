#include "iqrf/response/DPACoordBondNodeResponse.h"

using namespace BeeeOn;

static const int DPA_BOND_NODE_RESPONSE_SIZE = 2;

DPAMessage::NetworkAddress
	DPACoordBondNodeResponse::bondedNetworkAddress() const
{
	return peripheralData()[1];
}

size_t DPACoordBondNodeResponse::count() const
{
	return peripheralData()[0];
}
