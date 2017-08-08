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
BEEEON_OBJECT_REF("distributor", &PressureSensorManager::setDistributor)
BEEEON_OBJECT_REF("commandDispatcher", &PressureSensorManager::setCommandDispatcher)
BEEEON_OBJECT_TIME("refresh", &PressureSensorManager::setRefresh)
BEEEON_OBJECT_TEXT("path", &PressureSensorManager::setPath)
BEEEON_OBJECT_TEXT("vendor", &PressureSensorManager::setVendor)
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
	DeviceManager(DevicePrefix::PREFIX_PRESSURE_SENSOR),
	m_paired(false),
	m_refresh(15 * Timespan::SECONDS),
	m_vendor("BeeeOn")
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
		if (!m_paired) {
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
		m_paired = true;
}

bool PressureSensorManager::accept(const Command::Ptr cmd)
{
	if (cmd->is<GatewayListenCommand>())
		return true;
	if (cmd->is<DeviceAcceptCommand>())
		return cmd->cast<DeviceAcceptCommand>().deviceID().prefix() == DevicePrefix::PREFIX_PRESSURE_SENSOR;
	if (cmd->is<DeviceUnpairCommand>())
		return cmd->cast<DeviceUnpairCommand>().deviceID().prefix() == DevicePrefix::PREFIX_PRESSURE_SENSOR;

	return false;
}

void PressureSensorManager::handle(const Command::Ptr cmd, Answer::Ptr answer)
{
	if (cmd->is<GatewayListenCommand>())
		handleListenCommand(cmd->cast<GatewayListenCommand>(), answer);
	else if (cmd->is<DeviceAcceptCommand>())
		handleAcceptCommand(cmd->cast<DeviceAcceptCommand>(), answer);
	else if (cmd->is<DeviceUnpairCommand>())
		handleUnpairCommand(cmd->cast<DeviceUnpairCommand>(), answer);
	else
		throw IllegalStateException("received unaccepted command");
}

void PressureSensorManager::handleListenCommand(const GatewayListenCommand &, Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);

	if (!m_paired) {
		dispatch(new NewDeviceCommand(
			pairedID(),
			m_vendor,
			PRODUCT,
			TYPES,
			m_refresh));
	}
	result->setStatus(Result::Status::SUCCESS);
}

void PressureSensorManager::handleAcceptCommand(const DeviceAcceptCommand &cmd, Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);

	if (cmd.deviceID() != pairedID()) {
		poco_warning(logger(), "not accepting device with unknown id: "
			+ cmd.deviceID().toString());
		result->setStatus(Result::Status::FAILED);
		return;
	}

	if (!m_paired) {
		m_paired = true;
		m_event.set();
	}
	else {
		poco_warning(logger(), "ignoring accept of already paired device");
	}

	result->setStatus(Result::Status::SUCCESS);
}

void PressureSensorManager::handleUnpairCommand(const DeviceUnpairCommand &cmd, Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);

	if (cmd.deviceID() != pairedID()) {
		poco_warning(logger(), "not unpairing device with unknown id: "
			+ cmd.deviceID().toString());
		result->setStatus(Result::Status::FAILED);
		return;
	}

	if (m_paired) {
		m_paired = false;
		result->setStatus(Result::Status::SUCCESS);
	}
	else {
		poco_warning(logger(), "ignoring unpair of not paired device");
		result->setStatus(Result::Status::FAILED);
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

void PressureSensorManager::shipValue()
{
	SensorData data;

	data.setDeviceID(pairedID());

	try {
		double value;
		FileInputStream fStream(m_path);
		fStream >> value;
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

DeviceID PressureSensorManager::buildID(const string &path)
{
	const uint64_t h = Poco::hash(path);
	return DeviceID(DevicePrefix::PREFIX_PRESSURE_SENSOR, h & DeviceID::IDENT_MASK);
}
