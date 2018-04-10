#include <Poco/NumberFormatter.h>
#include <Poco/StringTokenizer.h>

#include "iqrf/DPAResponse.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const int DPA_RESPONSE_HEADER_SIZE = 8;
static const int DPA_MAX_MESSAGE_SIZE = DPA_RESPONSE_HEADER_SIZE + 59;

DPAResponse::DPAResponse():
	DPAMessage(0, 0, 0, 0, {}),
	m_errorCode(0),
	m_dpaValue(0)
{
}

DPAResponse::DPAResponse(
		DPAMessage::NetworkAddress networkAddress,
		uint8_t pNumber,
		uint8_t pCommand,
		uint16_t hwPID,
		const vector<uint8_t> &pData,
		uint8_t errorCode,
		uint8_t dpaValue):
	DPAMessage(networkAddress, pNumber, pCommand, hwPID, pData),
	m_errorCode(errorCode),
	m_dpaValue(dpaValue)
{
}

void DPAResponse::setErrorCode(uint8_t errCode)
{
	m_errorCode = errCode;
}

uint8_t DPAResponse::errorCode() const
{
	return m_errorCode;
}

void DPAResponse::setDPAValue(uint8_t dpaValue)
{
	m_dpaValue = dpaValue;
}

uint8_t DPAResponse::dpaValue() const
{
	return m_dpaValue;
}

DPAResponse::Ptr DPAResponse::fromRaw(const string &data)
{
	const StringTokenizer tokens(data, ".");
	vector<uint8_t> pData;

	if (tokens.count() < DPA_RESPONSE_HEADER_SIZE) {
		throw RangeException(
			"dpa header is shorter than minimum size of header "
			+ to_string(DPA_RESPONSE_HEADER_SIZE));
	}

	if (tokens.count() > DPA_MAX_MESSAGE_SIZE) {
		throw InvalidArgumentException(
			"dpa message is longer than "
			+ to_string(DPA_MAX_MESSAGE_SIZE));
	}

	vector<uint8_t> dpa;
	for (size_t i = 0; i < tokens.count(); i++) {
		try {
			dpa.push_back(NumberParser::parseHex("0x" + tokens[i]));
		}
		catch (const SyntaxException &ex) {
			throw InvalidArgumentException(
				"invalid parse dpa byte " + tokens[i]);
		}
	}

	const uint8_t cmd = dpa.at(3);
	DPAResponse::Ptr response;

	switch (cmd) {
	default:
		response = new DPAResponse;
	}

	response->setNetworkAddress((dpa.at(1) << 8) | dpa.at(0));
	response->setPeripheralNumber(dpa.at(2));
	response->setPeripheralCommand(cmd);
	response->setHWPID(((dpa.at(5) << 8 ) | dpa.at(4)));
	response->setErrorCode(dpa.at(6));
	response->setDPAValue(dpa.at(7));
	response->setPeripheralData(
		{dpa.begin() + DPA_RESPONSE_HEADER_SIZE, dpa.end()}
	);

	return response;
}

string DPAResponse::toDPAString() const
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
	repr += ".";

	repr += NumberFormatter::formatHex(m_errorCode, 2);
	repr += ".";

	repr += NumberFormatter::formatHex(m_dpaValue, 2);

	for (const auto &data : peripheralData()) {
		repr += ".";
		repr +=  NumberFormatter::formatHex(data, 2);
	}

	return toLower(repr);
}
