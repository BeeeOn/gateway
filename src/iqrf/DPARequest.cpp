#include <Poco/NumberFormatter.h>

#include "iqrf/DPARequest.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const uint8_t DPA_REQUEST_HEADER_SIZE = 6;

const uint8_t DPARequest::DPA_COORD_PNUM = 0x00;
const uint8_t DPARequest::DPA_NODE_PNUM = 0x01;
const uint8_t DPARequest::DPA_OS_PNUM = 0x02;

DPARequest::DPARequest():
	DPAMessage(0, 0, 0, 0, {})
{
}

DPARequest::DPARequest(
		DPAMessage::NetworkAddress node,
		uint8_t pNumber,
		uint8_t pCommand,
		uint16_t hwPID,
		const vector<uint8_t> &pData):
	DPAMessage(node, pNumber, pCommand, hwPID, pData)
{
}

DPARequest::DPARequest(
		DPAMessage::NetworkAddress node,
		uint8_t pNumber,
		uint8_t pCommand):
	DPAMessage(node, pNumber, pCommand, DEFAULT_HWPID, {})
{
}

string DPARequest::toDPAString() const
{
	string repr;

	repr += NumberFormatter::formatHex(networkAddress() & 0xff, 2);
	repr += ".";
	repr += NumberFormatter::formatHex(networkAddress() >> 8, 2);
	repr += ".";

	repr += NumberFormatter::formatHex(peripheralNumber(), 2);
	repr += ".";

	repr += NumberFormatter::formatHex(peripheralCommand(), 2);
	repr += ".";

	repr += NumberFormatter::formatHex(HWPID() & 0xff, 2);
	repr += ".";
	repr += NumberFormatter::formatHex(HWPID() >> 8, 2);

	for (const auto &data : peripheralData()) {
		repr += ".";
		repr +=  NumberFormatter::formatHex(data, 2);
	}

	return toLower(repr);
}

size_t DPARequest::size() const
{
	return DPA_REQUEST_HEADER_SIZE + peripheralData().size();
}
