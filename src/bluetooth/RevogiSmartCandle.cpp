#include <Poco/Exception.h>

#include "bluetooth/RevogiSmartCandle.h"
#include "bluetooth/HciConnection.h"

#define ON_OFF_MODULE_ID 0
#define BRIGHTNESS_MODULE_ID 1
#define COLOR_MODULE_ID 2

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const list<ModuleType> LIGHT_MODULE_TYPES = {
	{ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_BRIGHTNESS, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_COLOR, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
};

const string RevogiSmartCandle::LIGHT_NAME = "Delite-ED33";

RevogiSmartCandle::RevogiSmartCandle(const MACAddress& address, const Timespan& timeout):
	RevogiRGBLight(address, timeout, LIGHT_NAME, LIGHT_MODULE_TYPES)
{
}

RevogiSmartCandle::~RevogiSmartCandle()
{
}

void RevogiSmartCandle::requestModifyState(
		const ModuleID& moduleID,
		const double value,
		const HciInterface::Ptr hci)
{
	SynchronizedObject::ScopedLock guard(*this);

	HciConnection::Ptr conn = hci->connect(m_address, m_timeout);
	vector<unsigned char> actualSetting = conn->notifiedWrite(
		ACTUAL_VALUES_GATT, WRITE_VALUES_GATT, NOTIFY_DATA, m_timeout);

	if (actualSetting.size() != 18)
		throw ProtocolException("expected 18 B, received " + to_string(actualSetting.size()) + " B");

	switch (moduleID.value()) {
	case ON_OFF_MODULE_ID:
		modifyStatus(value, conn);
		break;
	case BRIGHTNESS_MODULE_ID:
		modifyBrightness(value, retrieveRGB(actualSetting), conn);
		break;
	case COLOR_MODULE_ID:
		modifyColor(value, conn);
		break;
	default:
		throw InvalidArgumentException("invalid module ID: " + to_string(moduleID.value()));
	}
}

SensorData RevogiSmartCandle::parseValues(const std::vector<unsigned char>& values) const
{
	if (values.size() != 18)
		throw ProtocolException("expected 18 B, received " + to_string(values.size()) + " B");

	double onOff = values[7] > 0xc8 ? 0 : 1;
	double brightness = brightnessToPercents(values[7]);
	uint32_t rgb = retrieveRGB(values);

	return {
		m_deviceId,
		Timestamp{},
		{
			{ON_OFF_MODULE_ID, onOff},
			{BRIGHTNESS_MODULE_ID, brightness},
			{COLOR_MODULE_ID, double(rgb)},
		}
	};
}
