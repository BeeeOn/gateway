#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>
#include <Poco/String.h>

#include "bluetooth/BluezHciInterface.h"
#include "bluetooth/DBusHciConnection.h"
#include "bluetooth/DBusHciInterface.h"
#include "di/Injectable.h"

BEEEON_OBJECT_BEGIN(BeeeOn, DBusHciInterfaceManager)
BEEEON_OBJECT_CASTABLE(HciInterfaceManager)
BEEEON_OBJECT_END(BeeeOn, DBusHciInterfaceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static int CHANGE_POWER_ATTEMPTS = 5;
static Timespan CHANGE_POWER_DELAY = 200 * Timespan::MILLISECONDS;
static int GERROR_IN_PROGRESS = 36;
static uint16_t RSSI_DEVICE_UNAVAILABLE = 0;
static Timespan MAX_AGE_RSSI = 90 * Timespan::SECONDS;

DBusHciInterface::DBusHciInterface(const string& name):
	m_name(name),
	m_loopThread(*this, &DBusHciInterface::runLoop)
{
	m_adapter = retrieveBluezAdapter(createAdapterPath(m_name));
	m_objectManager = createBluezObjectManager();

	for (auto& one : processKnownDevices(m_objectManager)) {
		const auto handle = ::g_signal_connect(
			one.raw(),
			"g-properties-changed",
			G_CALLBACK(onDeviceRSSIChanged),
			&m_devices);

		Device device(one, handle);
		m_devices.second.emplace(device.macAddress(), device);
	}

	m_objectManagerHandle = ::g_signal_connect(
		G_DBUS_OBJECT_MANAGER(m_objectManager.raw()),
		"object-added",
		G_CALLBACK(onDBusObjectAdded),
		&m_devices);

	m_thread.start(m_loopThread);
}

DBusHciInterface::~DBusHciInterface()
{
	stopDiscovery(m_adapter);

	::g_signal_handler_disconnect(m_objectManager.raw(), m_objectManagerHandle);
	for (auto& one : m_devices.second)
		::g_signal_handler_disconnect(one.second.device().raw(), one.second.rssiHandle());

	if (::g_main_loop_is_running(m_loop.raw()))
		::g_main_loop_quit(m_loop.raw());

	try {
		m_thread.join();
	}
	BEEEON_CATCH_CHAIN(logger())
}

/**
 * Convert the given GError into the appropriate exception and throw it.
 */
static void throwErrorIfAny(const GlibPtr<GError> error)
{
	if (!error.isNull()) {
		// This error occured when the discovery or connection is already in progress.
		if (error->code == GERROR_IN_PROGRESS)
			return;
		else
			throw IOException(error->message);
	}
}

void DBusHciInterface::up() const
{
	if (logger().debug())
		logger().debug("bringing up " + m_name, __FILE__, __LINE__);

	ScopedLock<FastMutex> guard(m_statusMutex);

	if (!::org_bluez_adapter1_get_powered(m_adapter.raw())) {
		::org_bluez_adapter1_set_powered(m_adapter.raw(), true);
		waitUntilPoweredChange(m_adapter, true);
	}

	startDiscovery(m_adapter, "le");
}

void DBusHciInterface::down() const
{
	if (logger().debug())
		logger().debug("switching down " + m_name, __FILE__, __LINE__);

	ScopedLock<FastMutex> guard(m_statusMutex);

	m_resetCondition.broadcast();

	if (!::org_bluez_adapter1_get_powered(m_adapter.raw()))
		return;

	::org_bluez_adapter1_set_powered(m_adapter.raw(), false);
	waitUntilPoweredChange(m_adapter, false);
}

void DBusHciInterface::reset() const
{
	down();
	up();
}

bool DBusHciInterface::detect(const MACAddress &address) const
{
	BluezHciInterface bluezHci(m_name);
	return bluezHci.detect(address);
}

map<MACAddress, string> DBusHciInterface::scan() const
{
	BluezHciInterface bluezHci(m_name);
	return bluezHci.scan();
}

map<MACAddress, string> DBusHciInterface::lescan(const Timespan& timeout) const
{
	logger().information("starting BLE scan for " +
		to_string(timeout.totalSeconds()) + " seconds", __FILE__, __LINE__);

	map<MACAddress, string> foundDevices;

	startDiscovery(m_adapter, "le");

	if (m_resetCondition.tryWait(timeout)) {
		if (logger().debug()) {
			logger().debug("the lescan was terminated prematurely",
				__FILE__, __LINE__);
		}
	}

	ScopedLock<FastMutex> guard(m_devices.first);
	for (auto one : m_devices.second) {
		if (one.second.lastSeen().elapsed() > MAX_AGE_RSSI.totalMicroseconds())
			continue;

		const auto rssi = one.second.rssi();
		if (rssi == RSSI_DEVICE_UNAVAILABLE)
			continue;

		const string name = one.second.name();
		foundDevices.emplace(one.first, name);

		if (logger().debug()) {
			logger().debug("found BLE device " + name +
				" by address " + one.first.toString(':') +
				" (" + to_string(rssi) + ")",
				__FILE__, __LINE__);
		}
	}

	logger().information("BLE scan has finished, found " +
		to_string(foundDevices.size()) + " device(s)", __FILE__, __LINE__);

	return foundDevices;
}

HciInfo DBusHciInterface::info() const
{
	BluezHciInterface bluezHci(m_name);
	return bluezHci.info();
}

HciConnection::Ptr DBusHciInterface::connect(
		const MACAddress& address,
		const Timespan& timeout) const
{
	if (logger().debug())
		logger().debug("connecting to device " + address.toString(':'), __FILE__, __LINE__);

	const string path = createDevicePath(m_name, address);
	GlibPtr<OrgBluezDevice1> device = retrieveBluezDevice(path);

	if (!::org_bluez_device1_get_connected(device.raw())) {
		GlibPtr<GError> error;
		::g_dbus_proxy_set_default_timeout(G_DBUS_PROXY(device.raw()), timeout.totalMilliseconds());
		::org_bluez_device1_call_connect_sync(device.raw(), nullptr, &error);

		throwErrorIfAny(error);
	}

	return new DBusHciConnection(m_name, device, timeout);
}

void DBusHciInterface::watch(
		const MACAddress& address,
		SharedPtr<WatchCallback> callBack)
{
	ScopedLock<FastMutex> lock(m_watchMutex);

	auto it = m_watchedDevices.find(address);
	if (it != m_watchedDevices.end())
		return;

	if (logger().debug())
		logger().debug("watch the device " + address.toString(':'), __FILE__, __LINE__);

	GlibPtr<OrgBluezDevice1> device = retrieveBluezDevice(createDevicePath(m_name, address));

	uint64_t handle = ::g_signal_connect(
		device.raw(),
		"g-properties-changed",
		G_CALLBACK(onDeviceManufacturerDataRecieved),
		callBack.get());

	if (handle == 0)
		throw IOException("failed to watch device " + address.toString());

	WatchedDevice watchedDevice(device, handle, callBack);
	m_watchedDevices.emplace(address, watchedDevice);
}

void DBusHciInterface::unwatch(const MACAddress& address)
{
	ScopedLock<FastMutex> lock(m_watchMutex);

	auto it = m_watchedDevices.find(address);
	if (it == m_watchedDevices.end())
		return;

	if (logger().debug())
		logger().debug("unwatch the device " + address.toString(':'), __FILE__, __LINE__);

	GlibPtr<OrgBluezDevice1> device = it->second.device();
	const auto handle = it->second.signalHandle();
	m_watchedDevices.erase(address);
	::g_signal_handler_disconnect(device.raw(), handle);
}

void DBusHciInterface::waitUntilPoweredChange(GlibPtr<OrgBluezAdapter1> adapter, const bool powered) const
{
	for (int i = 0; i < CHANGE_POWER_ATTEMPTS; ++i) {
		if (::org_bluez_adapter1_get_powered(adapter.raw()) == powered)
			return;

		m_condition.tryWait(m_statusMutex, CHANGE_POWER_DELAY.totalMilliseconds());
	}

	throw TimeoutException("failed to change power of interface" + m_name);
}

void DBusHciInterface::startDiscovery(
		GlibPtr<OrgBluezAdapter1> adapter,
		const std::string& trasport) const
{
	ScopedLock<FastMutex> guard(m_discoveringMutex);

	if (::org_bluez_adapter1_get_discovering(adapter.raw()))
		return;

	GlibPtr<GError> error;
	initDiscoveryFilter(adapter, trasport);
	::org_bluez_adapter1_call_start_discovery_sync(adapter.raw(), nullptr, &error);
	throwErrorIfAny(error);
}

void DBusHciInterface::stopDiscovery(GlibPtr<OrgBluezAdapter1> adapter) const
{
	ScopedLock<FastMutex> guard(m_discoveringMutex);

	if (!::org_bluez_adapter1_get_discovering(adapter.raw()))
		return;

	::org_bluez_adapter1_call_stop_discovery_sync(adapter.raw(), nullptr, nullptr);
}

void DBusHciInterface::initDiscoveryFilter(
		GlibPtr<OrgBluezAdapter1> adapter,
		const string& trasport) const
{
	GlibPtr<GError> error;
	GVariantBuilder args;
	::g_variant_builder_init(&args, G_VARIANT_TYPE("a{sv}"));
	::g_variant_builder_add(&args, "{sv}", "Transport", g_variant_new_string(trasport.c_str()));
	::org_bluez_adapter1_call_set_discovery_filter_sync(
		adapter.raw(), ::g_variant_builder_end(&args), nullptr, &error);
	throwErrorIfAny(error);
}

vector<GlibPtr<OrgBluezDevice1>> DBusHciInterface::processKnownDevices(
		GlibPtr<GDBusObjectManager> objectManager) const
{
	GlibPtr<GError> error;
	vector<GlibPtr<OrgBluezDevice1>> devices;
	PathFilter pathFilter =
		[&](const string& path)-> bool {
			if (path.find("/" + m_name) == string::npos)
				return true;
			else
				return false;
		};

	for (auto path : retrievePathsOfBluezObjects(objectManager, pathFilter, "org.bluez.Device1")) {
		GlibPtr<OrgBluezDevice1> device;
		try {
			device = retrieveBluezDevice(path.c_str());
		}
		BEEEON_CATCH_CHAIN_ACTION(logger(),
			continue);

		devices.emplace_back(device);
	}

	return devices;
}

void DBusHciInterface::runLoop()
{
	m_loop = ::g_main_loop_new(nullptr, false);
	::g_main_loop_run(m_loop.raw());
}

vector<string> DBusHciInterface::retrievePathsOfBluezObjects(
		GlibPtr<GDBusObjectManager> objectManager,
		PathFilter pathFilter,
		const std::string& objectFilter)
{
	vector<string> paths;
	GlibPtr<GList> objects = ::g_dbus_object_manager_get_objects(objectManager.raw());

	for (GList* l = objects.raw(); l != nullptr; l = l->next) {
		const string objectPath = ::g_dbus_object_get_object_path(G_DBUS_OBJECT(l->data));

		// Example of input: /org/bluez/hci0/dev_FF_FF_FF_FF_FF_FF
		if (pathFilter(objectPath))
			continue;

		GlibPtr<GDBusInterface> interface =
			::g_dbus_object_manager_get_interface(objectManager.raw(), objectPath.c_str(), objectFilter.c_str());
		if (interface.isNull())
			continue;

		paths.emplace_back(objectPath);
	}

	return paths;
}

gboolean DBusHciInterface::onStopLoop(gpointer loop)
{
	::g_main_loop_quit(reinterpret_cast<GMainLoop*>(loop));
	return false;
}

void DBusHciInterface::onDBusObjectAdded(
		GDBusObjectManager* objectManager,
		GDBusObject* object,
		gpointer userData)
{
	const string path = ::g_dbus_object_get_object_path(G_DBUS_OBJECT(object));
	GlibPtr<GDBusInterface> interface =
		::g_dbus_object_manager_get_interface(objectManager, path.c_str(), "org.bluez.Device1");
	if (interface.isNull())
		return;

	GlibPtr<OrgBluezDevice1> device;
	try {
		device = retrieveBluezDevice(path);
	}
	BEEEON_CATCH_CHAIN_ACTION(Loggable::forClass(typeid(DBusHciInterface)),
		return);

	const auto handle = ::g_signal_connect(
		device.raw(),
		"g-properties-changed",
		G_CALLBACK(onDeviceRSSIChanged),
		userData);

	ThreadSafeDevices &devices = *(reinterpret_cast<ThreadSafeDevices*>(userData));

	ScopedLock<FastMutex> guard(devices.first);
	Device newDevice(device, handle);
	devices.second.emplace(newDevice.macAddress(), newDevice);
}

gboolean DBusHciInterface::onDeviceRSSIChanged(
		OrgBluezDevice1* device,
		GVariant* properties,
		const gchar* const*,
		gpointer userData)
{
	if (::g_variant_n_children(properties) == 0)
		return true;

	GVariantIter* iter;
	const char* property;
	GVariant* value;
	ThreadSafeDevices &devices = *(reinterpret_cast<ThreadSafeDevices*>(userData));

	::g_variant_get(properties, "a{sv}", &iter);
	while (::g_variant_iter_loop(iter, "{&sv}", &property, &value)) {
		if (string(property) == "RSSI") {
			ScopedLock<FastMutex> guard(devices.first);
			MACAddress mac = MACAddress::parse(::org_bluez_device1_get_address(device), ':');
			auto it = devices.second.find(mac);
			if (it != devices.second.end())
				it->second.updateLastSeen();

			break;
		}
	}

	return true;
}

gboolean DBusHciInterface::onDeviceManufacturerDataRecieved(
		OrgBluezDevice1* device,
		GVariant* properties,
		const gchar* const*,
		gpointer userData)
{
	if (::g_variant_n_children(properties) == 0)
		return true;

	GVariantIter* iter;
	const char* property;
	GVariant* value;

	::g_variant_get(properties, "a{sv}", &iter);
	while (::g_variant_iter_loop(iter, "{&sv}", &property, &value)) {
		if (string(property) == "ManufacturerData") {
			processManufacturerData(device, value, userData);

			break;
		}
	}

	return true;
}

void DBusHciInterface::processManufacturerData(
		OrgBluezDevice1* device,
		GVariant* value,
		gpointer userData)
{
	GVariantIter* iter;
	GVariant* data;
	size_t size;

	::g_variant_get(value, "a{qv}", &iter);
	while (::g_variant_iter_loop(iter, "{qv}", nullptr, &data)) {
		const unsigned char* tmpData = reinterpret_cast<const unsigned char*>(
			::g_variant_get_fixed_array(data, &size, sizeof(unsigned char)));

		vector<unsigned char> result(tmpData, tmpData + size);
		const auto mac = MACAddress::parse(::org_bluez_device1_get_address(device), ':');

		WatchCallback &func = *(reinterpret_cast<WatchCallback*>(userData));
		func(mac, result);
	}
}

const string DBusHciInterface::createAdapterPath(const string& name)
{
	return "/org/bluez/" + name;
}

const string DBusHciInterface::createDevicePath(const string& name, const MACAddress& address)
{
	return "/org/bluez/" + name + "/dev_" + address.toString('_');
}

GlibPtr<GDBusObjectManager> DBusHciInterface::createBluezObjectManager()
{
	GlibPtr<GError> error;

	GlibPtr<GDBusObjectManager> objectManager = ::g_dbus_object_manager_client_new_for_bus_sync(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
		"org.bluez",
		"/",
		nullptr, nullptr, nullptr, nullptr,
		&error);

	throwErrorIfAny(error);
	return objectManager;
}

GlibPtr<OrgBluezAdapter1> DBusHciInterface::retrieveBluezAdapter(const string& path)
{
	GlibPtr<GError> error;

	GlibPtr<OrgBluezAdapter1> adapter = ::org_bluez_adapter1_proxy_new_for_bus_sync(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_PROXY_FLAGS_NONE,
		"org.bluez",
		path.c_str(),
		nullptr,
		&error);

	throwErrorIfAny(error);
	return adapter;
}

GlibPtr<OrgBluezDevice1> DBusHciInterface::retrieveBluezDevice(const string& path)
{
	GlibPtr<GError> error;

	GlibPtr<OrgBluezDevice1> device = ::org_bluez_device1_proxy_new_for_bus_sync(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_PROXY_FLAGS_NONE,
		"org.bluez",
		path.c_str(),
		nullptr,
		&error);

	throwErrorIfAny(error);
	return device;
}

DBusHciInterface::Device::Device(
		const GlibPtr<OrgBluezDevice1> device,
		const uint64_t rssiHandle):
	m_device(device),
	m_rssiHandle(rssiHandle)
{
}

string DBusHciInterface::Device::name()
{
	const char* charName = ::org_bluez_device1_get_name(m_device.raw());
	const string name = charName == nullptr ? "unknown" : charName;

	return name;
}

MACAddress DBusHciInterface::Device::macAddress()
{
	return MACAddress::parse(::org_bluez_device1_get_address(m_device.raw()), ':');
}

int16_t DBusHciInterface::Device::rssi()
{
	return ::org_bluez_device1_get_rssi(m_device.raw());
}

DBusHciInterface::WatchedDevice::WatchedDevice(
		const GlibPtr<OrgBluezDevice1> device,
		const uint64_t signalHandle,
		const Poco::SharedPtr<WatchCallback> callBack):
	m_device(device),
	m_signalHandle(signalHandle),
	m_callBack(callBack)
{
}


DBusHciInterfaceManager::DBusHciInterfaceManager()
{
}

HciInterface::Ptr DBusHciInterfaceManager::lookup(const string &name)
{
	ScopedLock<FastMutex> guard(m_mutex);

	auto it = m_interfaces.find(name);
	if (it != m_interfaces.end())
		return it->second;

	DBusHciInterface::Ptr newHci = new DBusHciInterface(name);
	m_interfaces.emplace(name, newHci);
	return newHci;
}
