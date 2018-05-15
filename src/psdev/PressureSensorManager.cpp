#include <fstream>

#include <Poco/Exception.h>
#include <Poco/FileStream.h>
#include <Poco/Hash.h>

#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "model/SensorData.h"
#include "model/SensorValue.h"
#include "psdev/PressureSensorManager.h"
#include "util/IncompleteTimestamp.h"

BEEEON_OBJECT_BEGIN(BeeeOn, PressureSensorManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_PROPERTY("distributor", &PressureSensorManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &PressureSensorManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("refresh", &PressureSensorManager::setRefresh)
BEEEON_OBJECT_PROPERTY("path", &PressureSensorManager::setPath)
BEEEON_OBJECT_PROPERTY("vendor", &PressureSensorManager::setVendor)
BEEEON_OBJECT_PROPERTY("unit", &PressureSensorManager::setUnit)
BEEEON_OBJECT_END(BeeeOn, PressureSensorManager)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

#define PRODUCT "Air Pressure Sensor"

static const list<ModuleType> TYPES =
	{
		{ModuleType::Type::TYPE_PRESSURE}
	};

PressureSensorManager::PressureSensorManager():
	DeviceManager(DevicePrefix::PREFIX_PRESSURE_SENSOR, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
	}),
	m_refresh(15 * Timespan::SECONDS),
	m_vendor("BeeeOn"),
	m_unit("kPa")
{
}

PressureSensorManager::~PressureSensorManager()
{
}

void PressureSensorManager::run()
{
	poco_information(logger(), "pressure sensor started");

	initialize();

	while(!m_stop) {
		if (!deviceCache()->paired(pairedID())) {
			m_event.wait();
			continue;
		}

		shipValue();
		m_event.tryWait(m_refresh.totalMilliseconds());
	}

	poco_information(logger(), "pressure sensor finished");
	m_stop = false;
}

void PressureSensorManager::stop()
{
	m_stop = true;
	m_event.set();
}

void PressureSensorManager::initialize()
{
	set<DeviceID> devices;
	try {
		devices = deviceList(-1);
	}
	catch (const Exception &ex) {
		logger().log(ex, __FILE__, __LINE__);
	}

	if (devices.size() > 1) {
		poco_warning(logger(),
			"obtained more than one paired sensor: "
			+ to_string(devices.size()));
	}

	if (devices.find(pairedID()) != devices.end())
		deviceCache()->markPaired(pairedID());
}

void PressureSensorManager::handleGeneric(const Command::Ptr cmd, Result::Ptr result)
{
	if (cmd->is<GatewayListenCommand>())
		handleListenCommand(cmd->cast<GatewayListenCommand>());
	else if (cmd->is<DeviceAcceptCommand>())
		handleAcceptCommand(cmd->cast<DeviceAcceptCommand>());
	else if (cmd->is<DeviceUnpairCommand>())
		handleUnpairCommand(cmd->cast<DeviceUnpairCommand>());
	else
		DeviceManager::handleGeneric(cmd, result);
}

void PressureSensorManager::handleListenCommand(const GatewayListenCommand &)
{
	if (!deviceCache()->paired(pairedID())) {
		dispatch(new NewDeviceCommand(
			pairedID(),
			m_vendor,
			PRODUCT,
			TYPES,
			m_refresh));
	}
}

void PressureSensorManager::handleAcceptCommand(const DeviceAcceptCommand &cmd)
{
	if (cmd.deviceID() != pairedID())
		throw NotFoundException("accept: " + cmd.deviceID().toString());

	if (!deviceCache()->paired(pairedID())) {
		deviceCache()->markPaired(pairedID());
		m_event.set();
	}
	else {
		poco_warning(logger(), "ignoring accept of already paired device");
	}
}

void PressureSensorManager::handleUnpairCommand(const DeviceUnpairCommand &cmd)
{
	if (cmd.deviceID() != pairedID()) {
		poco_warning(logger(), "not unpairing device with unknown id: "
			+ cmd.deviceID().toString());
		return;
	}

	if (deviceCache()->paired(pairedID())) {
		deviceCache()->markUnpaired(pairedID());
	}
	else {
		poco_warning(logger(), "ignoring unpair of not paired device");
	}
}

void PressureSensorManager::setRefresh(const Timespan &refresh)
{
	if (refresh < 0)
		throw InvalidArgumentException("refresh time must be positive");

	m_refresh = refresh;
}

void PressureSensorManager::setPath(const string &path)
{
	m_path = path;
}

void PressureSensorManager::setVendor(const string &vendor)
{
	m_vendor = vendor;
}

void PressureSensorManager::setUnit(const string &unit)
{
	if (unit != "kPa" && unit != "Pa")
		throw InvalidArgumentException("unknown unit specified");

	m_unit = unit;
}

void PressureSensorManager::shipValue()
{
	SensorData data;

	data.setDeviceID(pairedID());

	try {
		double value;
		FileInputStream fStream(m_path);
		fStream >> value;
		value = convertToHPA(value);
		data.insertValue(SensorValue(ModuleID(0), value));
		ship(data);
	}
	catch (const Exception &ex) {
		logger().log(ex, __FILE__, __LINE__);
	}
}

DeviceID PressureSensorManager::pairedID()
{
	return buildID(m_path);
}

double PressureSensorManager::convertToHPA(const double value)
{
	if (m_unit == "kPa") {
		return value * 10.0;
	}
	if (m_unit == "Pa") {
		return value / 100.0;
	}

	throw IllegalStateException("unknown unit set");
}

DeviceID PressureSensorManager::buildID(const string &path)
{
	const uint64_t h = Poco::hash(path);
	return DeviceID(DevicePrefix::PREFIX_PRESSURE_SENSOR, h & DeviceID::IDENT_MASK);
}
