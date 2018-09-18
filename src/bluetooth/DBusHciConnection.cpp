#include <Poco/Event.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "bluetooth/DBusHciConnection.h"
#include "bluetooth/DBusHciInterface.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const string GATT_CHARACTERISTIC = "org.bluez.GattCharacteristic1";

/**
 * Convert the given GError into the appropriate exception and throw it.
 */
static void throwErrorIfAny(const GlibPtr<GError> error)
{
	if (!error.isNull())
		throw IOException(error->message);
}

DBusHciConnection::DBusHciConnection(
		const string& hciName,
		GlibPtr<OrgBluezDevice1> device,
		const Timespan& timeout):
	m_hciName(hciName),
	m_device(device),
	m_timeout(timeout)
{
	m_address = MACAddress::parse(::org_bluez_device1_get_address(m_device.raw()), ':');
	resolveServices();
}

DBusHciConnection::~DBusHciConnection()
{
	disconnect();
}

vector<unsigned char> DBusHciConnection::read(const UUID& uuid)
{
	if (logger().debug()) {
		logger().debug("sending read request to device " + m_address.toString(':'),
			__FILE__, __LINE__);
	}

	GlibPtr<OrgBluezGattCharacteristic1> characteristic = findGATTCharacteristic(uuid);
	if (characteristic.isNull())
		throw NotFoundException("no such GATT characteristic " + uuid.toString());

	::g_dbus_proxy_set_default_timeout(G_DBUS_PROXY(characteristic.raw()), m_timeout.totalMilliseconds());

	GlibPtr<GVariant> requiredValue;
	GVariantBuilder args;
	::g_variant_builder_init(&args, G_VARIANT_TYPE("a{sv}"));
	GlibPtr<GError> error;
	::org_bluez_gatt_characteristic1_call_read_value_sync(
		characteristic.raw(), ::g_variant_builder_end(&args), &requiredValue, nullptr, &error);

	throwErrorIfAny(error);

	gsize size = 0;
	const unsigned char* data = reinterpret_cast<const unsigned char*>(
		::g_variant_get_fixed_array(requiredValue.raw(), &size, sizeof(unsigned char)));
	vector<unsigned char> result(data, data + size);

	return result;
}

void DBusHciConnection::write(
		const UUID& uuid,
		const vector<unsigned char>& value)
{
	if (logger().debug()) {
		logger().debug("sending write request to device " + m_address.toString(':'),
			__FILE__, __LINE__);
	}

	doWrite(uuid, value);
}

void DBusHciConnection::resolveServices()
{
	if (logger().debug()) {
		logger().debug("resolving the services of device " + m_address.toString(':'),
			__FILE__, __LINE__);
	}

	// services are already loaded
	if (::org_bluez_device1_get_services_resolved(m_device.raw()))
		return;

	Event resolved;

	uint32_t handle = ::g_signal_connect(
		m_device.raw(),
		"g-properties-changed",
		G_CALLBACK(onDeviceServicesResolved),
		&resolved);

	resolved.tryWait(m_timeout.totalMilliseconds());

	::g_signal_handler_disconnect(m_device.raw(), handle);

	if (!::org_bluez_device1_get_services_resolved(m_device.raw()))
		throw TimeoutException("resolving of services failed");
}

void DBusHciConnection::doWrite(
		const Poco::UUID& uuid,
		const std::vector<unsigned char>& value)
{
	GlibPtr<OrgBluezGattCharacteristic1> characteristic = findGATTCharacteristic(uuid);
	if (characteristic.isNull())
		throw NotFoundException("no such GATT characteristic " + uuid.toString());

	::g_dbus_proxy_set_default_timeout(G_DBUS_PROXY(characteristic.raw()), m_timeout.totalMilliseconds());

	GVariant* data = ::g_variant_new_from_data(
		G_VARIANT_TYPE("ay"), &value[0], value.size(), true, nullptr, nullptr);
	GVariantBuilder args;
	::g_variant_builder_init(&args, G_VARIANT_TYPE("a{sv}"));
	GlibPtr<GError> error;
	::org_bluez_gatt_characteristic1_call_write_value_sync(
		characteristic.raw(), data, ::g_variant_builder_end(&args), nullptr, &error);

	throwErrorIfAny(error);
}

gboolean DBusHciConnection::onDeviceServicesResolved(
		OrgBluezDevice1*,
		GVariant* properties,
		const gchar* const*,
		gpointer userData)
{
	if (::g_variant_n_children(properties) == 0)
		return true;

	Event &resolved = *(reinterpret_cast<Event*>(userData));
	GVariantIter* iter;
	const char* key;
	GVariant* value;

	::g_variant_get(properties, "a{sv}", &iter);
	while (::g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
		if (string(key) == "ServicesResolved") {
			resolved.set();

			break;
		}
	}

	return true;
}

void DBusHciConnection::disconnect()
{
	if (logger().debug()) {
		logger().debug("disconnecting device " + m_address.toString(':'),
			__FILE__, __LINE__);
	}

	::org_bluez_device1_call_disconnect_sync(m_device.raw(), nullptr, nullptr);
}

GlibPtr<OrgBluezGattCharacteristic1> DBusHciConnection::findGATTCharacteristic(
		const UUID& uuid)
{
	GlibPtr<OrgBluezGattCharacteristic1> characteristic;

	GlibPtr<GDBusObjectManager> objectManager = DBusHciInterface::createBluezObjectManager();
	function<bool(const string& path)> pathFilter =
		[&](const string& path)-> bool {
			if (path.find(m_hciName + "/dev_" + m_address.toString('_')) == string::npos)
				return true;
			else
				return false;
		};

	for (auto path : DBusHciInterface::retrievePathsOfBluezObjects(objectManager, pathFilter, GATT_CHARACTERISTIC)) {
		try {
			characteristic = retrieveBluezGATTCharacteristic(path);
		}
		BEEEON_CATCH_CHAIN_ACTION(logger(),
			continue);

		const string charUUID = ::org_bluez_gatt_characteristic1_get_uuid(characteristic.raw());
		if (charUUID == uuid.toString())
			return characteristic;
	}

	return nullptr;
}

GlibPtr<OrgBluezGattCharacteristic1> DBusHciConnection::retrieveBluezGATTCharacteristic(
	const string& path)
{
	GlibPtr<GError> error;

	GlibPtr<OrgBluezGattCharacteristic1> characteristic = ::org_bluez_gatt_characteristic1_proxy_new_for_bus_sync(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_PROXY_FLAGS_NONE,
		"org.bluez",
		path.c_str(),
		nullptr,
		&error);

	throwErrorIfAny(error);
	return characteristic;
}
