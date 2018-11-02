#include <vector>

#include <Poco/Clock.h>
#include <Poco/Message.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Thread.h>

#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "jablotron/JablotronDeviceManager.h"
#include "hotplug/HotplugEvent.h"
#include "model/SensorData.h"
#include "util/BlockingAsyncWork.h"
#include "util/UnsafePtr.h"

BEEEON_OBJECT_BEGIN(BeeeOn, JablotronDeviceManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(HotplugListener)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &JablotronDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("distributor", &JablotronDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &JablotronDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("txBackOffFactory", &JablotronDeviceManager::setTxBackOffFactory)
BEEEON_OBJECT_PROPERTY("unpairErasesSlot", &JablotronDeviceManager::setUnpairErasesSlot)
BEEEON_OBJECT_PROPERTY("pgyEnrollGap", &JablotronDeviceManager::setPGYEnrollGap)
BEEEON_OBJECT_PROPERTY("eraseAllOnProbe", &JablotronDeviceManager::setEraseAllOnProbe)
BEEEON_OBJECT_PROPERTY("registerOnProbe", &JablotronDeviceManager::setRegisterOnProbe)
BEEEON_OBJECT_PROPERTY("maxProbeAttempts", &JablotronDeviceManager::setMaxProbeAttempts)
BEEEON_OBJECT_PROPERTY("probeTimeout", &JablotronDeviceManager::setProbeTimeout)
BEEEON_OBJECT_PROPERTY("ioJoinTimeout", &JablotronDeviceManager::setIOJoinTimeout)
BEEEON_OBJECT_PROPERTY("ioReadTimeout", &JablotronDeviceManager::setIOReadTimeout)
BEEEON_OBJECT_PROPERTY("ioErrorSleep", &JablotronDeviceManager::setIOErrorSleep)
BEEEON_OBJECT_END(BeeeOn, JablotronDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const int MAX_GADGETS_COUNT = 32;

static set<unsigned int> allSlots()
{
	set<unsigned int> slots;

	for (unsigned int i = 0; i < MAX_GADGETS_COUNT; ++i)
		slots.emplace(i);

	return slots;
}

static const Timespan SHORT_TIMEOUT = 1 * Timespan::SECONDS;
static const Timespan ERASE_ALL_TIMEOUT = 5 * Timespan::SECONDS;
static const Timespan SCAN_SLOTS_TIMEOUT = 15 * Timespan::SECONDS;
static const Timespan CONTROLLER_ERROR_SLEEP = 1 * Timespan::SECONDS;

static const DeviceID PGX_ID = {DevicePrefix::PREFIX_JABLOTRON, 0x00ffffffffffff01};
static const DeviceID PGY_ID = {DevicePrefix::PREFIX_JABLOTRON, 0x00ffffffffffff02};
static const DeviceID SIREN_ID = {DevicePrefix::PREFIX_JABLOTRON, 0x00ffffffffffff03};

static const ModuleID SIREN_ALARM_ID = 0;
static const ModuleID SIREN_BEEP_ID = 1;

static list<ModuleType> PG_MODULES = {
	{ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
};

static list<ModuleType> SIREN_MODULES = {
	{ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_ENUM, {"SIREN_BEEPING"}, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
};

static string addressToString(const uint32_t address)
{
	return NumberFormatter::format0(address, 8)
		+ " [" + NumberFormatter::formatHex(address, 6) + "]";
}

JablotronDeviceManager::JablotronDeviceManager():
	DeviceManager(DevicePrefix::PREFIX_JABLOTRON, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceSetValueCommand),
		typeid(DeviceSearchCommand),
	}),
	m_unpairErasesSlot(false),
	m_pgyEnrollGap(4 * Timespan::SECONDS), // determined experimentally
	m_pgx(false),
	m_pgy(false),
	m_alarm(false),
	m_beep(JablotronController::BEEP_NONE)
{
}

void JablotronDeviceManager::setUnpairErasesSlot(bool erase)
{
	m_unpairErasesSlot = erase;
}

void JablotronDeviceManager::setTxBackOffFactory(BackOffFactory::Ptr factory)
{
	m_txBackOffFactory = factory;
}

void JablotronDeviceManager::setPGYEnrollGap(const Timespan &gap)
{
	if (gap < 0)
		throw InvalidArgumentException("pgyEnrollGap must be non-negative");

	m_pgyEnrollGap = gap;
}

void JablotronDeviceManager::setEraseAllOnProbe(bool erase)
{
	m_eraseAllOnProbe = erase;
}

void JablotronDeviceManager::setRegisterOnProbe(const list<string> &addresses)
{
	list<uint32_t> registerOnProbe;

	for (const auto &address : addresses) {
		registerOnProbe.emplace_back(
			NumberParser::parseUnsigned(address));
	}

	m_registerOnProbe = registerOnProbe;
}

void JablotronDeviceManager::setMaxProbeAttempts(int count)
{
	m_controller.setMaxProbeAttempts(count);
}

void JablotronDeviceManager::setProbeTimeout(const Timespan &timeout)
{
	m_controller.setProbeTimeout(timeout);
}

void JablotronDeviceManager::setIOJoinTimeout(const Timespan &timeout)
{
	m_controller.setIOJoinTimeout(timeout);
}

void JablotronDeviceManager::setIOReadTimeout(const Timespan &timeout)
{
	m_controller.setIOReadTimeout(timeout);
}

void JablotronDeviceManager::setIOErrorSleep(const Timespan &delay)
{
	m_controller.setIOErrorSleep(delay);
}

DeviceID JablotronDeviceManager::buildID(uint32_t address)
{
	const auto primary = JablotronGadget::Info::primaryAddress(address);

	return DeviceID(DevicePrefix::PREFIX_JABLOTRON, primary);
}

uint32_t JablotronDeviceManager::extractAddress(const DeviceID &id)
{
	return id.ident() & 0x0ffffff;
}

void JablotronDeviceManager::sleep(const Timespan &delay)
{
	const Clock started;

	while (true) {
		Timespan remaining = delay - started.elapsed();
		if (remaining < 0)
			return;

		if (remaining < 1 * Timespan::MILLISECONDS)
			remaining = 1 * Timespan::MILLISECONDS;

		Thread::sleep(remaining.totalMilliseconds());
	}
}

void JablotronDeviceManager::onAdd(const HotplugEvent &e)
{
	const auto dev = hotplugMatch(e);
	if (dev.empty())
		return;

	FastMutex::ScopedLock guard(m_lock);

	m_controller.probe(dev);
	initDongle();
	syncSlots();
}

void JablotronDeviceManager::onRemove(const HotplugEvent &e)
{
	const auto dev = hotplugMatch(e);
	if (dev.empty())
		return;

	m_controller.release(dev);
}

string JablotronDeviceManager::hotplugMatch(const HotplugEvent &e)
{
	if (!e.properties()->has("tty.BEEEON_DONGLE"))
		return "";

	const auto &dongle = e.properties()->getString("tty.BEEEON_DONGLE");

	if (dongle != "jablotron")
		return "";

	return e.node();
}

void JablotronDeviceManager::run()
{
	UnsafePtr<Thread>(Thread::current())->setName("reporting");

	StopControl::Run run(m_stopControl);

	while (run) {
		try {
			const auto report = m_controller.pollReport(-1);
			if (!report)
				continue;

			if (logger().debug()) {
				logger().debug(
					"shipping report " + report.toString(),
					__FILE__, __LINE__);
			}

			const auto id = buildID(report.address);

			if (!deviceCache()->paired(id)) {
				if (logger().debug()) {
					logger().debug(
						"skipping report from unpaired device " + id.toString(),
						__FILE__, __LINE__);
				}

				continue;
			}

			shipReport(report);
		}
		BEEEON_CATCH_CHAIN(logger())
	}
}

void JablotronDeviceManager::stop()
{
	answerQueue().dispose();
	DeviceManager::stop();
	m_controller.dispose();
}

void JablotronDeviceManager::handleRemoteStatus(
	const DevicePrefix &prefix,
	const set<DeviceID> &devices,
	const DeviceStatusHandler::DeviceValues &values)
{
	FastMutex::ScopedLock guard(m_lock);

	DeviceManager::handleRemoteStatus(prefix, devices, values);
	syncSlots();
}

vector<JablotronGadget> JablotronDeviceManager::readGadgets(
		const Timespan &timeout)
{
	const Clock started;
	vector<JablotronGadget> gadgets;

	for (unsigned int i = 0; i < MAX_GADGETS_COUNT; ++i) {
		Timespan remaining = timeout - started.elapsed();
		if (remaining < 0)
			throw TimeoutException("timeout exceeded while reading gadgets");

		const auto address = m_controller.readSlot(i, remaining);
		if (address.isNull()) {
			if (logger().trace()) {
				logger().trace(
					"slot " + to_string(i) + " is empty",
					__FILE__, __LINE__);
			}

			continue;
		}

		const auto info = JablotronGadget::Info::resolve(address);
		if (!info) {
			logger().warning(
				"unrecognized gadget address "
				+ addressToString(address));
		}

		gadgets.emplace_back(JablotronGadget{i, address, info});
	}

	return gadgets;
}

void JablotronDeviceManager::scanSlots(
		set<uint32_t> &registered,
		set<unsigned int> &freeSlots,
		set<unsigned int> &unknownSlots)
{
	registered.clear();
	freeSlots = allSlots();
	unknownSlots.clear();

	for (const auto &gadget : readGadgets(SCAN_SLOTS_TIMEOUT)) {
		freeSlots.erase(gadget.slot());

		if (!gadget.info()) {
			logger().warning(
				"discovered registered unknown gadget: " + gadget.toString()
				+ " " + buildID(gadget.address()).toString(),
				__FILE__, __LINE__);

			unknownSlots.emplace(gadget.slot());
			continue;
		}

		registered.emplace(gadget.address());

		logger().information(
			"discovered registered gadget: " + gadget.toString()
			+ " " + buildID(gadget.address()).toString(),
			__FILE__, __LINE__);
	}
}

void JablotronDeviceManager::initDongle()
{
	if (m_eraseAllOnProbe) {
		logger().notice("erasing all slots after probe...");
		m_controller.eraseSlots(ERASE_ALL_TIMEOUT);
	}

	if (m_registerOnProbe.empty())
		return;

	logger().notice("registering slots after probe...");

	set<uint32_t> registered;
	set<unsigned int> freeSlots;
	set<unsigned int> unknownSlots;

	scanSlots(registered, freeSlots, unknownSlots);

	for (const auto &address : m_registerOnProbe) {
		if (registered.find(address) != registered.end()) {
			logger().information(
				addressToString(address) + " is already registered");
			continue;
		}

		if (!freeSlots.empty()) {
			registerGadget(freeSlots, address, SHORT_TIMEOUT);
		}
		else {
			logger().warning("overwriting a non-free slot...");
			registerGadget(unknownSlots, address, SHORT_TIMEOUT);
		}
	}
}

void JablotronDeviceManager::syncSlots()
{
	logger().information("syncing slots...");

	set<uint32_t> registered;
	set<unsigned int> freeSlots;
	set<unsigned int> unknownSlots;

	scanSlots(registered, freeSlots, unknownSlots);

	const set<DeviceID> paired = deviceCache()->paired(prefix());

	for (const auto &id : paired) {
		if (id == PGX_ID || id == PGY_ID || id == SIREN_ID) {
			// ignore, nothing to sync for those
			continue;
		}

		const auto primary = extractAddress(id);
		const auto secondary = JablotronGadget::Info::secondaryAddress(primary);

		if (logger().debug()) {
			logger().debug(
				"try sync gadget " + addressToString(primary)
				+ " (secondary: " + NumberFormatter::format0(secondary, 8) + "): "
				+ id.toString(),
				__FILE__, __LINE__);
		}

		if (registered.find(primary) == registered.end()) {
			if (!freeSlots.empty()) {
				registerGadget(freeSlots, primary, SHORT_TIMEOUT);
			}
			else {
				logger().warning("overwriting a non-free slot...");
				registerGadget(unknownSlots, primary, SHORT_TIMEOUT);
			}
		}
		else {
			if (logger().debug()) {
				logger().debug(
					"device " + id.toString() + " is already registered",
					__FILE__, __LINE__);
			}
		}

		if (primary == secondary) {
			if (logger().debug()) {
				logger().debug(
					"device " + id.toString() + " is not dual, continue",
					__FILE__, __LINE__);
			}

			continue;
		}

		if (registered.find(secondary) == registered.end()) {
			if (!freeSlots.empty()) {
				registerGadget(freeSlots, secondary, SHORT_TIMEOUT);
			}
			else {
				logger().warning("overwriting a non-free slot...");
				registerGadget(unknownSlots, primary, SHORT_TIMEOUT);
			}
		}
	}
}

void JablotronDeviceManager::shipReport(const JablotronReport &report)
{
	const auto info = JablotronGadget::Info::resolve(report.address);
	if (!info) {
		logger().warning(
			"unrecognized device by address "
			+ addressToString(report.address));
		return;
	}

	const Timestamp now;
	const auto values = info.parse(report);

	if (!values.empty()) {
		if (logger().debug()) {
			logger().debug(
				"shipping data from " + info.name(),
				__FILE__, __LINE__);
		}

		ship({buildID(report.address), now, values});
	}
}

void JablotronDeviceManager::registerGadget(
		set<unsigned int> &freeSlots,
		const uint32_t address,
		const Timespan &timeout)
{
	const auto targetSlot = freeSlots.begin();
	if (targetSlot == freeSlots.end()) {
		throw IllegalStateException(
			"no free slots available to register "
			+ addressToString(address));
	}

	logger().information(
		"registering address " + addressToString(address)
		+ " at slot " + to_string(*targetSlot),
		__FILE__, __LINE__);

	m_controller.registerSlot(*targetSlot, address, timeout);
	freeSlots.erase(targetSlot);
}

void JablotronDeviceManager::acceptGadget(const DeviceID &id)
{
	FastMutex::ScopedLock guard(m_lock);

	set<uint32_t> registered;
	set<unsigned int> freeSlots = allSlots();

	for (const auto &gadget : readGadgets(SCAN_SLOTS_TIMEOUT)) {
		registered.emplace(gadget.address());
		freeSlots.erase(gadget.slot());
	}

	const auto address = extractAddress(id);

	const auto it = registered.find(address);
	if (it == registered.end())
		registerGadget(freeSlots, address, SHORT_TIMEOUT);

	const auto secondary = JablotronGadget::Info::secondaryAddress(address);

	if (secondary != address) {
		const auto it = registered.find(secondary);

		if (it == registered.end())
			registerGadget(freeSlots, secondary, SHORT_TIMEOUT);
	}
}

void JablotronDeviceManager::enrollTX()
{
	m_controller.sendEnroll(SHORT_TIMEOUT);
}

void JablotronDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	const auto id = cmd->deviceID();

	if (id == PGX_ID || id == SIREN_ID) {
		enrollTX();
	}
	else if (id == PGY_ID) {
		enrollTX();
		sleep(m_pgyEnrollGap);
		enrollTX();
	}
	else {
		acceptGadget(id);
	}

	DeviceManager::handleAccept(cmd);
}

void JablotronDeviceManager::newDevice(
		const DeviceID &id,
		const string &name,
		const list<ModuleType> &types,
		const Timespan &refreshTime)
{
	auto builder = DeviceDescription::Builder()
		.id(id)
		.type("Jablotron", name)
		.modules(types);

	if (refreshTime < 0)
		builder.noRefreshTime();
	else
		builder.refreshTime(refreshTime);

	dispatch(new NewDeviceCommand(builder.build()));
}

AsyncWork<>::Ptr JablotronDeviceManager::startDiscovery(const Timespan &timeout)
{
	const Clock started;

	if (!deviceCache()->paired(PGX_ID))
		newDevice(PGX_ID, "PGX", PG_MODULES, -1);
	if (!deviceCache()->paired(PGY_ID))
		newDevice(PGY_ID, "PGY", PG_MODULES, -1);
	if (!deviceCache()->paired(SIREN_ID))
		newDevice(SIREN_ID, "Siren", SIREN_MODULES, -1);

	FastMutex::ScopedLock guard(m_lock);

	for (const auto &gadget : readGadgets(timeout)) {
		if (!gadget.info())
			continue;

		const auto id = buildID(gadget.address());
		const bool paired = deviceCache()->paired(id);

		logger().information(
			"gadget (" + string(paired? "paired" : "not-paired") + "): "
			+ gadget.toString() + " " + id.toString(),
			__FILE__, __LINE__);

		if (paired)
			continue;

		if (gadget.isSecondary())
			continue; // secondary devices are not reported

		newDevice(id, gadget.info().name(),
			gadget.info().modules, gadget.info().refreshTime);
	}

	return BlockingAsyncWork<>::instance();
}

AsyncWork<>::Ptr JablotronDeviceManager::startSearch(
		const Timespan &timeout,
		const uint64_t serialNumber)
{
	if (serialNumber > 0xffffffff) {
		throw InvalidArgumentException(
			"address " + to_string(serialNumber)
			+ " is out-of range");
	}

	const uint32_t address = static_cast<uint32_t>(serialNumber);
	const auto info = JablotronGadget::Info::resolve(address);
	if (!info) {
		throw InvalidArgumentException(
			"address " + NumberFormatter::format0(address, 8)
			+ " was not recognized");
	}

	const auto id = buildID(address);

	FastMutex::ScopedLock guard(m_lock);

	set<uint32_t> registered;
	set<unsigned int> freeSlots;
	set<unsigned int> unknownSlots;

	scanSlots(registered, freeSlots, unknownSlots);

	if (registered.find(address) == registered.end()) {
		if (!freeSlots.empty()) {
			registerGadget(freeSlots, address, timeout);
		}
		else {
			logger().warning("overwriting a non-free slot...", __FILE__, __LINE__);
			registerGadget(unknownSlots, address, timeout);
		}
	}

	if (!deviceCache()->paired(id))
		newDevice(id, info.name(), info.modules, info.refreshTime);

	return BlockingAsyncWork<>::instance();
}

void JablotronDeviceManager::unregisterGadget(
		const DeviceID &id,
		const Timespan &timeout)
{
	if (!m_unpairErasesSlot) {
		if (logger().debug()) {
			logger().debug(
				"unregistering of gadgets from slots is disabled",
				__FILE__, __LINE__);
		}

		return;
	}

	const uint32_t address = extractAddress(id);
	const uint32_t secondary = JablotronGadget::Info::secondaryAddress(address);
	bool done = false;

	FastMutex::ScopedLock guard(m_lock);

	for (const auto &gadget : readGadgets(timeout)) {
		if (address != gadget.address() && secondary != gadget.address())
			continue;

		m_controller.unregisterSlot(gadget.slot(), SHORT_TIMEOUT);

		logger().information(
			"gadget " + gadget.toString()
			+ " was unregistered from its slot");

		done = true;
	}

	if (!done)
		logger().warning("device " + id.toString() + " was not registered in any slot");
}

AsyncWork<set<DeviceID>>::Ptr JablotronDeviceManager::startUnpair(
		const DeviceID &id,
		const Timespan &timeout)
{
	set<DeviceID> result;

	logger().information("unpairing device " + id.toString());

	if (id == PGX_ID || id == PGY_ID || id == SIREN_ID) {
		 // almost nothing to do, un-enroll does not exist
		deviceCache()->markUnpaired(id);
		result.emplace(id);
	}
	else {
		unregisterGadget(id, timeout);
		deviceCache()->markUnpaired(id);
		result.emplace(id);
	}

	auto work = BlockingAsyncWork<set<DeviceID>>::instance();
	work->setResult(result);

	return work;
}

AsyncWork<double>::Ptr JablotronDeviceManager::startSetValue(
		const DeviceID &id,
		const ModuleID &module,
		const double value,
		const Timespan &timeout)
{
	if (!deviceCache()->paired(id)) {
		throw NotFoundException(
			"no such device " + id.toString() + " is paired");
	}

	if (id != PGX_ID && id != PGY_ID && id != SIREN_ID) {
		throw InvalidArgumentException(
			"device " + id.toString() + " is not controllable");
	}

	if ((id == PGX_ID || id == PGY_ID) && module != ModuleID(0)) {
		throw NotFoundException(
			"no such controllable module " + module.toString()
			+ " for device " + id.toString());
	}

	if (value < 0) {
		throw InvalidArgumentException(
			"invalid value for device "
			+ id.toString() + ": " + to_string(value));
	}

	const auto v = static_cast<unsigned int>(value);

	if (id == PGX_ID) {
		m_pgx = v != 0;
	}
	else if (id == PGY_ID) {
		m_pgy = v != 0;
	}
	else if (id == SIREN_ID) {
		if (module == SIREN_ALARM_ID) {
			m_alarm = v != 0;
		}
		else if (module != SIREN_BEEP_ID) {
			throw NotFoundException(
				"no such controllable module " + module.toString()
				+ " for device " + id.toString());
		}

		switch (v) {
		case 0:
			m_beep = JablotronController::BEEP_NONE;
			break;
		case 1:
			m_beep = JablotronController::BEEP_SLOW;
			break;
		case 2:
			m_beep = JablotronController::BEEP_FAST;
			break;
		default:
			throw InvalidArgumentException(
				"invalid value for beep control: " + to_string(value));
		}
	}

	BackOff::Ptr backoff = m_txBackOffFactory->create();
	Timespan delay;

	do {
		m_controller.sendTX(m_pgx, m_pgy, m_alarm, m_beep, timeout);
		delay = backoff->next();
		sleep(delay);
	} while (delay != BackOff::STOP);

	auto work = BlockingAsyncWork<double>::instance();
	work->setResult(static_cast<double>(v));
	return work;
}
