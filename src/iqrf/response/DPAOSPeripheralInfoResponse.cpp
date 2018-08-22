#include <Poco/NumberFormatter.h>

#include "iqrf/response/DPAOSPeripheralInfoResponse.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

uint32_t DPAOSPeripheralInfoResponse::mid() const
{
	uint32_t moduleID = 0;

	moduleID |= peripheralData()[0];
	moduleID |= peripheralData()[1] << 8;
	moduleID |= peripheralData()[2] << 16;
	moduleID |= peripheralData()[3] << 24;

	return moduleID;
}

int8_t DPAOSPeripheralInfoResponse::rssi() const
{
	if (peripheralData()[8] < 11 ||peripheralData()[8] > 141) {
		throw RangeException(
			"supply voltage value "
			+ NumberFormatter::formatHex(peripheralData()[8], true));
	}

	return peripheralData()[8] - 130;
}

double DPAOSPeripheralInfoResponse::supplyVoltage() const
{
	if (peripheralData()[9] > 59) {
		throw RangeException(
			"supply voltage value "
			+ NumberFormatter::formatHex(peripheralData()[9], true));
	}

	return 261.12 / (127 - peripheralData()[9]);
}

double DPAOSPeripheralInfoResponse::percentageSupplyVoltage() const
{
	if (peripheralData()[9] > 59) {
		throw RangeException(
			"supply voltage value "
			+ NumberFormatter::formatHex(peripheralData()[9], true)
			+ " is out of range");
	}

	return (100.0 / 59.0) * peripheralData()[9];
}
