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
#include "util/BlockingAsyncWork.h"
#include "util/IncompleteTimestamp.h"

BEEEON_OBJECT_BEGIN(BeeeOn, PressureSensorManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &PressureSensorManager::setDeviceCache)
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

	StopControl::Run run(m_stopControl);

	while(run) {
		if (!deviceCache()->paired(pairedID())) {
			run.waitStoppable(-1);
			continue;
		}

		shipValue();
		run.waitStoppable(m_refresh);
	}

	poco_information(logger(), "pressure sensor finished");
}

void PressureSensorManager::stop()
{
	DeviceManager::stop();
}

void PressureSensorManager::handleRemoteStatus(
		const DevicePrefix &prefix,
		const set<DeviceID> &devices,
		const DeviceStatusHandler::DeviceValues &values)
{
	DeviceManager::handleRemoteStatus(prefix, devices, values);
	m_stopControl.requestWakeup();
}

AsyncWork<>::Ptr PressureSensorManager::startDiscovery(const Timespan &)
{
	if (!deviceCache()->paired(pairedID())) {
		const auto description = DeviceDescription::Builder()
			.id(pairedID())
			.type(m_vendor, PRODUCT)
			.modules(TYPES)
			.refreshTime(m_refresh)
			.build();

		dispatch(new NewDeviceCommand(description));
	}

	return BlockingAsyncWork<>::instance();
}

void PressureSensorManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	if (cmd->deviceID() != pairedID())
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	if (!deviceCache()->paired(pairedID())) {
		deviceCache()->markPaired(pairedID());
		m_stopControl.requestWakeup();
	}
	else {
		poco_warning(logger(), "ignoring accept of already paired device");
		return;
	}
	DeviceManager::handleAccept(cmd);
}

AsyncWork<set<DeviceID>>::Ptr PressureSensorManager::startUnpair(
		const DeviceID &id,
		const Timespan &)
{
	auto work = BlockingAsyncWork<set<DeviceID>>::instance();

	if (id != pairedID()) {
		poco_warning(logger(), "not unpairing device with unknown id: "
			+ id.toString());

		return work;
	}

	if (deviceCache()->paired(pairedID())) {
		deviceCache()->markUnpaired(pairedID());
		work->setResult({id});
	}
	else {
		poco_warning(logger(), "ignoring unpair of not paired device");
	}

	return work;
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
