#include <Poco/Exception.h>

#include "bluetooth/TabuLumenSmartLite.h"
#include "bluetooth/HciConnection.h"

#define ON_OFF_MODULE_ID 0
#define BRIGHTNESS_MODULE_ID 1
#define COLOR_MODULE_ID 2
#define MIN_COLOR 1
#define MAX_COLOR 16777215
#define MAX_COLOR_ELEMENT 0x63

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static list<ModuleType> LIGHT_MODULE_TYPES = {
	{ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_BRIGHTNESS, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_COLOR, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
};

const UUID TabuLumenSmartLite::WRITE_VALUES = UUID("0000fff1-0000-1000-8000-00805f9b34fb");
const vector<uint8_t> TabuLumenSmartLite::ADD_KEY = {
	0x00, 0xf4, 0xe5, 0xd6, 0xa3, 0xb2, 0xa3, 0xb2, 0xc1, 0xf4,
	0xe5, 0xd6, 0xa3, 0xb2, 0xc1, 0xf4, 0xe5, 0xd6, 0xa3, 0xb2};
const vector<uint8_t> TabuLumenSmartLite::XOR_KEY = {
	0x00, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0xf7, 0xe8, 0xd9, 0xca,
	0xbb, 0xac, 0x9d, 0x8e, 0x7f, 0x5e, 0x6f, 0xf7, 0xe8, 0xd9};
const string TabuLumenSmartLite::LIGHT_NAME = "TL 100S Smart Light";
const string TabuLumenSmartLite::VENDOR_NAME = "Tabu Lumen";

TabuLumenSmartLite::TabuLumenSmartLite(
		const MACAddress& address,
		const Timespan& timeout,
		const HciInterface::Ptr hci):
	BLESmartDevice(address, timeout, hci),
	m_colorBrightness(MAX_COLOR_ELEMENT, MAX_COLOR_ELEMENT, MAX_COLOR_ELEMENT, MAX_COLOR_ELEMENT)
{
}

TabuLumenSmartLite::~TabuLumenSmartLite()
{
}

std::list<ModuleType> TabuLumenSmartLite::moduleTypes() const
{
	return LIGHT_MODULE_TYPES;
}

std::string TabuLumenSmartLite::productName() const
{
	return LIGHT_NAME;
}

std::string TabuLumenSmartLite::vendor() const
{
	return VENDOR_NAME;
}

void TabuLumenSmartLite::requestModifyState(
	const ModuleID& moduleID,
	const double value)
{
	SynchronizedObject::ScopedLock guard(*this);

	switch (moduleID.value()) {
	case ON_OFF_MODULE_ID: {
		modifyStatus(value, m_hci);
		break;
	}
	case BRIGHTNESS_MODULE_ID: {
		modifyBrightness(value, m_hci);
		break;
	}
	case COLOR_MODULE_ID: {
		modifyColor(value, m_hci);
		break;
	}
	default:
		throw IllegalStateException("invalid module ID: " + to_string(moduleID.value()));
	}
}

bool TabuLumenSmartLite::match(const string& modelID)
{
	return modelID == "BG521";
}

void TabuLumenSmartLite::modifyStatus(const int64_t value, const HciInterface::Ptr hci)
{
	vector<uint8_t> data(20, 0);

	if (value != 0 && value != 1)
		throw IllegalStateException("value is not allowed");

	if (value == 1) {
		data[1] = m_colorBrightness.red();
		data[2] = m_colorBrightness.green();
		data[3] = m_colorBrightness.blue();
	}
	encryptMessage(data);
	if (value == 0)
		data[0] = Command::OFF;
	else
		data[0] = Command::ON_ACTION;

	writeData(data, hci);
}

void TabuLumenSmartLite::modifyBrightness(const int64_t value,const HciInterface::Ptr hci)
{
	ColorBrightness tmpColorBri(m_colorBrightness);
	vector<uint8_t> data(20, 0);

	if (value < 0 || value > 100)
		throw IllegalStateException("value is out of range");

	tmpColorBri.setBrightness(value);

	data[1] = tmpColorBri.red();
	data[2] = tmpColorBri.green();
	data[3] = tmpColorBri.blue();
	encryptMessage(data);
	data[0] = Command::ON_ACTION;

	writeData(data, hci);

	m_colorBrightness = tmpColorBri;
}

void TabuLumenSmartLite::modifyColor(const int64_t value,const HciInterface::Ptr hci)
{
	ColorBrightness tmpColorBri(m_colorBrightness);
	vector<uint8_t> data(20, 0);

	if (value < MIN_COLOR || value > MAX_COLOR)
		throw IllegalStateException("value is out of range");

	uint32_t rgb = value;
	tmpColorBri.setColor(
		(uint8_t(rgb >> 16) / 255.0) * MAX_COLOR_ELEMENT,
		(uint8_t(rgb >> 8) / 255.0) * MAX_COLOR_ELEMENT,
		(uint8_t(rgb) / 255.0) * MAX_COLOR_ELEMENT);

	data[1] = tmpColorBri.red();
	data[2] = tmpColorBri.green();
	data[3] = tmpColorBri.blue();
	encryptMessage(data);
	data[0] = Command::ON_ACTION;

	writeData(data, hci);

	m_colorBrightness = tmpColorBri;
}

void TabuLumenSmartLite::writeData(
		const vector<unsigned char>& data,
		const HciInterface::Ptr hci) const
{
	vector<unsigned char> authMsg = authorizationMessage();

	HciConnection::Ptr conn = hci->connect(m_address, m_timeout);
	conn->write(WRITE_VALUES, authMsg);
	conn->write(WRITE_VALUES, data);
}

vector<uint8_t> TabuLumenSmartLite::authorizationMessage() const
{
	vector<uint8_t> data = {
		0x00, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	encryptMessage(data);
	data[0] = Command::LOGIN;

	return data;
}

void TabuLumenSmartLite::encryptMessage(std::vector<uint8_t> &data) const
{
	int c = 0;
	for (int j = data.size() - 1; j >= 0; j--) {
		int k = c + data[j] + ADD_KEY[j];
		if (k >= 256) {
			c = k >> 8;
			k -= 256;
		}
		else {
			c = 0;
		}

		data[j] = k;
	}

	for (size_t i = 0; i < data.size(); i++)
		data[i] = data[i] ^ XOR_KEY[i];
}

void TabuLumenSmartLite::decryptMessage(std::vector<uint8_t> &data) const
{
	for (size_t i = 0; i < data.size(); i++)
		data[i] = data[i] ^ XOR_KEY[i];

	vector<uint8_t> dataCopy(data.size());
	copy(data.begin(), data.end(), dataCopy.begin());

	for (size_t i = 0; i < data.size() - 1; i++) {
		if (dataCopy[i + 1] < ADD_KEY[i + 1])
			dataCopy[i] = (0xff + dataCopy[i]);

		data[i] = dataCopy[i] - ADD_KEY[i];
	}
	data[data.size() - 1] = dataCopy[data.size() - 1] - ADD_KEY[data.size() - 1];
}
