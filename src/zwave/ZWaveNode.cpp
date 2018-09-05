#include <cmath>

#include <Poco/Exception.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/String.h>

#include "zwave/ZWaveNode.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

ZWaveNode::CommandClass::CommandClass(
		uint8_t id,
		uint8_t index,
		uint8_t instance,
		const string &name):
	m_id(id),
	m_index(index),
	m_instance(instance),
	m_name(name)
{
}

uint8_t ZWaveNode::CommandClass::id() const
{
	return m_id;
}

uint8_t ZWaveNode::CommandClass::index() const
{
	return m_index;
}

uint8_t ZWaveNode::CommandClass::instance() const
{
	return m_instance;
}

string ZWaveNode::CommandClass::name() const
{
	return m_name;
}

string ZWaveNode::CommandClass::toString() const
{
	string repr;

	repr = to_string(m_id) + ":" + to_string(m_index);

	if (m_instance)
		repr += "[" + to_string(m_instance) + "]";

	if (!m_name.empty())
		repr += " (" + m_name + ")";

	return repr;
}

bool ZWaveNode::CommandClass::operator <(const CommandClass &cc) const
{
	if (m_id < cc.m_id)
		return true;

	if (m_id > cc.m_id)
		return false;

	if (m_index < cc.m_index)
		return true;

	if (m_index > cc.m_index)
		return false;

	return m_instance < cc.m_instance;
}

ZWaveNode::Value::Value(
		const ZWaveNode &node,
		const CommandClass &cc,
		const string &value,
		const string &unit):
	Value(node.id(), cc, value, unit)
{
}

ZWaveNode::Value::Value(
		const Identity &node,
		const ZWaveNode::CommandClass &cc,
		const string &value,
		const string &unit):
	m_node(node),
	m_commandClass(cc),
	m_value(value),
	m_unit(unit)
{
}

const ZWaveNode::Identity &ZWaveNode::Value::node() const
{
	return m_node;
}

const ZWaveNode::CommandClass &ZWaveNode::Value::commandClass() const
{
	return m_commandClass;
}

string ZWaveNode::Value::value() const
{
	return m_value;
}

string ZWaveNode::Value::unit() const
{
	return m_unit;
}

bool ZWaveNode::Value::asBool() const
{
	return NumberParser::parseBool(m_value);
}

uint32_t ZWaveNode::Value::asHex32() const
{
	return NumberParser::parseHex(m_value);
}

double ZWaveNode::Value::asDouble() const
{
	return NumberParser::parseFloat(m_value);
}

int ZWaveNode::Value::asInt(bool floor) const
{
	if (!floor)
		return NumberParser::parse(m_value);

	int result;
	if (NumberParser::tryParse(m_value, result))
		return result;

	return ::floor(NumberParser::parseFloat(m_value));
}

double ZWaveNode::Value::asCelsius() const
{
	double v = NumberParser::parseFloat(m_value);

	if (m_unit == "F")
		return (5.0 * (v - 32.0)) / 9.0;

	if (m_unit == "C")
		return v;


	throw InvalidArgumentException(
		"unrecognized temperature unit: " + m_unit);
}

double ZWaveNode::Value::asLuminance() const
{
	double v = NumberParser::parseFloat(m_value);

	// convert percent to lux, consider 1000 lux as 100 %
	// https://github.com/CZ-NIC/domoticz-turris-gadgets/blob/master/hardware/OpenZWave.cpp#L1641
	if (m_unit == "%") {
		if (v >= 100.0)
			return 1000.0;

		return 10.0 * v;
	}
	if (m_unit == "lux")
		return v;

	throw InvalidArgumentException(
		"unrecognized luminance unit: " + m_unit);
}

double ZWaveNode::Value::asPM25() const
{
	if (!icompare(m_unit, "ug/m3"))
		return asDouble();

	throw InvalidArgumentException(
		"unrecognized PM2.5 unit: " + m_unit);
}

Timespan ZWaveNode::Value::asTime() const
{
	unsigned long t = NumberParser::parse(m_value);

	if (!icompare(m_unit, "seconds"))
		return t * Timespan::SECONDS;
	else
		throw InvalidArgumentException(
			"unrecognized time unit: " + m_unit);
}

string ZWaveNode::Value::toString() const
{
	return NumberFormatter::formatHex(m_node.home, 8)
		+ ":" +
		NumberFormatter::format(m_node.node)
		+ " "
		+ m_commandClass.toString()
		+ " "
		+ m_value
		+ " ["
		+ m_unit
		+ "]";
}

ZWaveNode::Identity::Identity(const uint32_t home, const uint8_t node):
	home(home),
	node(node)
{
}

/*
 * This method looks a bit insane. However, we are OK with assigning
 * full identity at once while preserving direct access to its members.
 */
ZWaveNode::Identity &ZWaveNode::Identity::operator =(const Identity &other)
{
	const_cast<uint32_t &>(this->home) = other.home;
	const_cast<uint8_t &>(this->node) = other.node;
	return *this;
}

bool ZWaveNode::Identity::operator !=(const Identity &other) const
{
	return home != other.home || node != other.node;
}

bool ZWaveNode::Identity::operator ==(const Identity &other) const
{
	return home == other.home && node == other.node;
}

bool ZWaveNode::Identity::operator <(const Identity &other) const
{
	if (home < other.home)
		return true;
	if (home > other.home)
		return false;

	return node < other.node;
}

std::string ZWaveNode::Identity::toString() const
{
        return NumberFormatter::formatHex(home, 8)
		+ ":"
		+ NumberFormatter::format(node);
}

ZWaveNode::ZWaveNode(const Identity &id, bool controller):
	m_id(id),
	m_controller(controller),
	m_queried(false),
	m_support(0),
	m_productId(0),
	m_vendorId(0)
{
}

uint32_t ZWaveNode::home() const
{
	return m_id.home;
}

uint8_t ZWaveNode::node() const
{
	return m_id.node;
}

const ZWaveNode::Identity &ZWaveNode::id() const
{
	return m_id;
}

bool ZWaveNode::controller() const
{
	return m_controller;
}

void ZWaveNode::setSupport(uint32_t support)
{
	m_support = support;
}

uint32_t ZWaveNode::support() const
{
	return m_support;
}

void ZWaveNode::setProductId(uint16_t id)
{
	m_productId = id;
}

uint16_t ZWaveNode::productId() const
{
	return m_productId;
}

void ZWaveNode::setProduct(const string &name)
{
	m_product = name;
}

string ZWaveNode::product() const
{
	return m_product;
}

void ZWaveNode::setProductType(uint16_t type)
{
	m_productType = type;
}

uint16_t ZWaveNode::productType() const
{
	return m_productType;
}

void ZWaveNode::setVendorId(uint16_t id)
{
	m_vendorId = id;
}

uint16_t ZWaveNode::vendorId() const
{
	return m_vendorId;
}

void ZWaveNode::setVendor(const string &vendor)
{
	m_vendor = vendor;
}

string ZWaveNode::vendor() const
{
	return m_vendor;
}

void ZWaveNode::setQueried(bool queried)
{
	m_queried = queried;
}

bool ZWaveNode::queried() const
{
	return m_queried;
}

void ZWaveNode::add(const CommandClass &cc)
{
	m_commandClasses.emplace(cc);
}

const set<ZWaveNode::CommandClass> &ZWaveNode::commandClasses() const
{
	return m_commandClasses;
}

string ZWaveNode::toString() const
{
        return m_id.toString();
}

string ZWaveNode::toInfoString() const
{
	string repr;

	if (!m_product.empty())
		repr += m_product;
	else
		repr += "none";

	repr += " (";
	repr += NumberFormatter::formatHex(m_productId, 4);
	repr += "/";
	repr += NumberFormatter::formatHex(m_productType, 4);
	repr += ")";

	repr += " ";

	if (!m_vendor.empty())
		repr += m_vendor;
	else
		repr += "none";

	if (m_vendorId != 0) {
		repr += " (";
		repr += NumberFormatter::formatHex(m_vendorId, 4);
		repr += ")";
	}

	repr += " [";

	if (m_support & SUPPORT_LISTENING)
		repr += "L";
	if (m_support & SUPPORT_BEAMING)
		repr += "B";
	if (m_support & SUPPORT_ROUTING)
		repr += "R";
	if (m_support & SUPPORT_SECURITY)
		repr += "S";
	if (m_support & SUPPORT_ZWAVEPLUS)
		repr += "+";
	if (m_controller)
		repr += "C";

	repr += "]";

	return repr;
}

bool ZWaveNode::operator <(const ZWaveNode &node) const
{
	return m_id < node.m_id;
}
