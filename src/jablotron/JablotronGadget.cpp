#include <vector>

#include <Poco/Exception.h>
#include <Poco/NumberFormatter.h>

#include "jablotron/JablotronGadget.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

static const RefreshTime REFRESH_TIME_9MIN = RefreshTime::fromMinutes(9);
static const RefreshTime REFRESH_TIME_NONE = RefreshTime::NONE;

static const uint32_t RC86K_FIRST = 0x800000;
static const uint32_t RC86K_LAST  = 0x87ffff;
static const uint32_t RC86K_DIFF  = 0x100000;

static const uint32_t RC86K_SECONDARY_FIRST = RC86K_FIRST + RC86K_DIFF;
static const uint32_t RC86K_SECONDARY_LAST  = RC86K_LAST + RC86K_DIFF;

const vector<JablotronGadget::Info> JablotronGadget::GADGETS = {
	{0xcf0000, 0xcfffff,   AC88, REFRESH_TIME_NONE, {
		{ModuleType::Type::TYPE_ON_OFF},
	}},
	{0x580000, 0x59ffff,  JA80L, REFRESH_TIME_NONE, {
		{ModuleType::Type::TYPE_ON_OFF},
		{ModuleType::Type::TYPE_SECURITY_ALERT},
		{ModuleType::Type::TYPE_SECURITY_ALERT},
	}},
	{0x180000, 0x1bffff,  JA81M, REFRESH_TIME_9MIN, {
		{ModuleType::Type::TYPE_OPEN_CLOSE},
		{ModuleType::Type::TYPE_SECURITY_ALERT},
		{ModuleType::Type::TYPE_BATTERY},
	}},
	{0x7f0000, 0x7fffff, JA82SH, REFRESH_TIME_9MIN, {
		{ModuleType::Type::TYPE_SHAKE},
		{ModuleType::Type::TYPE_SECURITY_ALERT},
		{ModuleType::Type::TYPE_BATTERY},
	}},
	{0x1c0000, 0x1dffff,  JA83M, REFRESH_TIME_9MIN, {
		{ModuleType::Type::TYPE_OPEN_CLOSE},
		{ModuleType::Type::TYPE_SECURITY_ALERT},
		{ModuleType::Type::TYPE_BATTERY},
	}},
	{0x640000, 0x65ffff,  JA83P, REFRESH_TIME_9MIN, {
		{ModuleType::Type::TYPE_MOTION},
		{ModuleType::Type::TYPE_SECURITY_ALERT},
		{ModuleType::Type::TYPE_BATTERY},
	}},
	{0x760000, 0x76ffff, JA85ST, REFRESH_TIME_9MIN, {
		{ModuleType::Type::TYPE_FIRE},
		{ModuleType::Type::TYPE_SECURITY_ALERT},
		{ModuleType::Type::TYPE_BATTERY},
	}},
	{RC86K_FIRST, RC86K_LAST, RC86K,  REFRESH_TIME_NONE, {
		{ModuleType::Type::TYPE_OPEN_CLOSE},
		{ModuleType::Type::TYPE_OPEN_CLOSE},
		{ModuleType::Type::TYPE_SECURITY_ALERT},
		{ModuleType::Type::TYPE_BATTERY},
	}},
	{0x240000, 0x25ffff, TP82N,  REFRESH_TIME_NONE, {
		{ModuleType::Type::TYPE_TEMPERATURE, {
			ModuleType::Attribute::ATTR_INNER,
			ModuleType::Attribute::ATTR_MANUAL_ONLY,
			ModuleType::Attribute::ATTR_CONTROLLABLE,
		}},
		{ModuleType::Type::TYPE_TEMPERATURE, {
			ModuleType::Attribute::ATTR_INNER
		}},
		{ModuleType::Type::TYPE_BATTERY},
	}},
};

JablotronGadget::Info::operator bool() const
{
	return firstAddress < lastAddress;
}

bool JablotronGadget::Info::operator !() const
{
	return !(firstAddress < lastAddress);
}

string JablotronGadget::Info::name() const
{
	switch (type) {
	case AC88:
		return "AC-88 (sensor)"; // the " (sensor)" is to handle incompatibility
	case JA80L:
		return "JA-80L";
	case JA81M:
		return "JA-81M";
	case JA82SH:
		return "JA-82SH";
	case JA83M:
		return "JA-83M";
	case JA83P:
		return "JA-83P";
	case JA85ST:
		return "JA-85ST";
	case RC86K:
		return "RC-86K (dual)";
	case TP82N:
		return "TP-82N";
	default:
		throw InvalidArgumentException(
			"unrecognized Jablotron device type: "
			+ to_string(type));
	}
}

JablotronGadget::Info JablotronGadget::Info::resolve(
		const uint32_t address)
{
	const auto primary = primaryAddress(address);

	for (const auto &gadget : GADGETS) {
		if (primary < gadget.firstAddress || gadget.lastAddress < primary)
			continue;

		return gadget;
	}

	static const Info invalid = {0, 0, NONE, RefreshTime::NONE, {}};
	return invalid;
}

vector<SensorValue> JablotronGadget::Info::parse(const JablotronReport &report) const
{
	vector<SensorValue> values;

	switch (type) {
	case JablotronGadget::AC88:
		values.push_back({0, static_cast<double>(report.get("RELAY"))});
		break;

	case JablotronGadget::JA80L:
		if (report.has("BUTTON"))
			values.push_back({0, 1.0});
		if (report.has("TAMPER"))
			values.push_back({1, 1.0});

		values.push_back({2, static_cast<double>(report.get("BLACKOUT"))});
		break;

	case JablotronGadget::JA81M:
		if (report.has("SENSOR"))
			values.push_back({0, static_cast<double>(report.get("ACT"))});
		if (report.has("TAMPER"))
			values.push_back({1, static_cast<double>(report.get("ACT"))});

		values.push_back({2, static_cast<double>(report.battery())});
		break;

	case JablotronGadget::JA83M:
		if (report.has("SENSOR"))
			values.push_back({0, static_cast<double>(report.get("ACT"))});
		if (report.has("TAMPER"))
			values.push_back({1, static_cast<double>(report.get("ACT"))});

		values.push_back({2, static_cast<double>(report.battery())});
		break;

	case JablotronGadget::JA82SH:
	case JablotronGadget::JA83P:
	case JablotronGadget::JA85ST:
		if (report.has("SENSOR"))
			values.push_back({0, 1.0});
		if (report.has("TAMPER"))
			values.push_back({1, static_cast<double>(report.get("ACT"))});

		values.push_back({2, static_cast<double>(report.battery())});
		break;

	case JablotronGadget::RC86K:
		if (!report.has("PANIC")) {
			const bool primary = report.address == primaryAddress(report.address);
			const ModuleID module = primary? 0 : 1;
			values.push_back({module, static_cast<double>(report.get("ARM"))});
		}
		else {
			values.push_back({2, 1.0});
		}

		values.push_back({3, static_cast<double>(report.battery())});
		break;

	case JablotronGadget::TP82N:
		if (report.has("INT", true))
			values.push_back({0, report.temperature("INT")});
		if (report.has("SET", true))
			values.push_back({1, report.temperature("SET")});

		values.push_back({2, static_cast<double>(report.battery())});
		break;

	case JablotronGadget::NONE:
		poco_assert(false);
	}

	return values;
}

uint32_t JablotronGadget::Info::primaryAddress(const uint32_t address)
{
	if (RC86K_SECONDARY_FIRST <= address && address <= RC86K_SECONDARY_LAST)
		return address - RC86K_DIFF;

	return address;
}

uint32_t JablotronGadget::Info::secondaryAddress(const uint32_t address)
{
	if (RC86K_FIRST <= address && address <= RC86K_LAST)
		return address + RC86K_DIFF;

	return address;
}

JablotronGadget::JablotronGadget(
		const unsigned int slot,
		const uint32_t address,
		const Info &info):
	m_slot(slot),
	m_address(address),
	m_info(info)
{
}

unsigned int JablotronGadget::slot() const
{
	return m_slot;
}

uint32_t JablotronGadget::address() const
{
	return m_address;
}

const JablotronGadget::Info &JablotronGadget::info() const
{
	return m_info;
}

bool JablotronGadget::isSecondary() const
{
	return Info::primaryAddress(m_address) != m_address;
}

string JablotronGadget::toString() const
{
	const string name = m_info ? m_info.name() : "<unknown>";

	return "SLOT:" + NumberFormatter::format0(m_slot, 2)
		+ " [" + NumberFormatter::format0(m_address, 8) + "] "
		+ name;
}
