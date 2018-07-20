#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <Poco/Clock.h>
#include <Poco/Error.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "di/Injectable.h"
#include "bluetooth/BluezHciInterface.h"
#include "io/AutoClose.h"

BEEEON_OBJECT_BEGIN(BeeeOn, BluezHciInterfaceManager)
BEEEON_OBJECT_CASTABLE(HciInterfaceManager)
BEEEON_OBJECT_END(BeeeOn, BluezHciInterfaceManager)

#define EIR_NAME_SHORT 0x08    // shortened local name
#define EIR_NAME_COMPLETE 0x09  // complete local name
#define LE_DISABLE 0x00
#define LE_ENABLE 0x01
#define LE_FILTER 0x00
#define LE_FILTER_DUP 1
#define LE_INTERVAL 0x0010
#define LE_OWN_TYPE 0x00
#define LE_TO 1000
#define LE_TYPE 0x01
#define LE_WINDOW 0x0010

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const int INQUIRY_LENGTH = 8; // ~10 seconds
static const int MAX_RESPONSES = 255;

namespace BeeeOn {

struct HciClose {
	void operator() (const int sock) const
	{
		if (sock >= 0)
			::hci_close_dev(sock);
	}
};

class HciAutoClose : public AutoClose<int, HciClose> {
public:
	HciAutoClose(const int sock) :
		AutoClose<int, HciClose>(m_sock),
		m_sock(sock)
	{}

private:
	int m_sock;
};

}

BluezHciInterface::BluezHciInterface(const std::string &name) :
	m_name(name)
{
}

/**
 * Convert the given errno value into the appropriate exception
 * and throw it with own message prefix.
 */
static void throwFromErrno(const int e, const string &prefix = "")
{
	if (prefix.empty())
		throw IOException(::strerror(e));
	else
		throw IOException(prefix + ": " + ::strerror(e));
}

static evt_le_meta_event* skipHciEventHdr(char *data, const unsigned &size)
{
	if (1 + HCI_EVENT_HDR_SIZE >= size) {
		throw IllegalStateException(
			"size of buffer is bigger than 1+HCI_EVENT_HDR_SIZE");
	}

	char *ptr = data + (1 + HCI_EVENT_HDR_SIZE);
	return (evt_le_meta_event *) ptr;
}

int BluezHciInterface::hciSocket() const
{
	int sock = ::socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (sock < 0)
		throwFromErrno(errno);

	return sock;
}

int BluezHciInterface::findHci(const std::string &name) const
{
	FdAutoClose sock(hciSocket());
	return findHci(*sock, name);
}

static struct hci_dev_info findHciInfo(int sock, const string name)
{
	struct hci_dev_list_req *list;
	struct hci_dev_req *req;

	/*
	 * The kernel API expects buffer in the following format:
	 * | count | hci_dev_req | hci_dev_req | ...
	 *    2 B        ...
	 */
	list = (struct hci_dev_list_req*) ::malloc(
			sizeof(uint16_t) + HCI_MAX_DEV * sizeof(*req));
	if (list == NULL)
		throw OutOfMemoryException("failed to alloc list of HCI devices");

	list->dev_num = HCI_MAX_DEV;
	req = list->dev_req;

	if (::ioctl(sock, HCIGETDEVLIST, (void *) list) < 0) {
		const int e = errno;
		::free(list);
		throwFromErrno(e);
	}

	for (int i = 0; i < list->dev_num; ++i) {
		struct hci_dev_info info;
		::memset(&info, 0, sizeof(info));
		info.dev_id = req[i].dev_id;

		if (::ioctl(sock, HCIGETDEVINFO, (void *) &info) < 0) {
			Loggable::forMethod(__func__)
				.error("ioctl(HCIGETDEVINFO): " + string(strerror(errno)),
					__FILE__, __LINE__);
			continue;
		}

		const string dev_name(info.name);
		if (dev_name == name) {
			::free(list);
			return info;
		}
	}

	::free(list);
	throw NotFoundException("no such HCI interface: " + name);
}

int BluezHciInterface::findHci(int sock, const std::string &name) const
{
	struct hci_dev_info info = findHciInfo(sock, name);
	return info.dev_id;
}

void BluezHciInterface::up() const
{
	FdAutoClose sock(hciSocket());
	struct hci_dev_info info = findHciInfo(*sock, m_name);

	logger().debug("bringing up " + m_name, __FILE__, __LINE__);

	if (::hci_test_bit(HCI_UP, &info.flags))
		return; // already UP

	if (::ioctl(*sock, HCIDEVUP, info.dev_id)) {
		if (errno != EALREADY)
			throwFromErrno(errno);
	}
}

void BluezHciInterface::reset() const
{
	FdAutoClose sock(hciSocket());
	struct hci_dev_info info = findHciInfo(*sock, m_name);

	logger().debug("resetting " + m_name, __FILE__, __LINE__);
	if (::ioctl(*sock, HCIDEVRESET, info.dev_id) < 0) {
		if (errno != EALREADY)
			throwFromErrno(errno, "reset of " + m_name + " failed");
	}
}

bool BluezHciInterface::detect(const MACAddress &address) const
{
	logger().debug("trying to detect device " + address.toString(':'),
			__FILE__, __LINE__);

	const int dev = findHci(m_name);
	const int sock = ::hci_open_dev(dev);
	if (sock < 0)
		throwFromErrno(errno);

	bdaddr_t addr;
	address.into(addr.b);

	char name[248];
	::memset(name, 0, sizeof(name));

	const int ret = ::hci_read_remote_name(sock, &addr, sizeof(name), name, 0);

	::hci_close_dev(sock); // close as soon as possible

	if (ret < 0) {
		if (errno == EIO)
			logger().debug("missing device " + address.toString(':'));
		else
			logger().error(::strerror(errno), __FILE__, __LINE__);

		return false;
	}

	logger().debug("detected device " + string(name) + " by address " + address.toString(':'));
	return true;
}

map<MACAddress, string> BluezHciInterface::scan() const
{
	const int dev = findHci(m_name);

	logger().debug("starting inquiry", __FILE__, __LINE__);

	inquiry_info *info = NULL;

	const int count = ::hci_inquiry(
		dev, INQUIRY_LENGTH, MAX_RESPONSES, NULL, &info, IREQ_CACHE_FLUSH);
	if (count < 0)
		throwFromErrno(errno);

	logger().debug("received " + to_string(count) + " responses",
			__FILE__, __LINE__);

	map<MACAddress, string> devices;

	if (count == 0)
		return devices; // empty

	const int sock = ::hci_open_dev(dev);
	if (sock < 0)
		throwFromErrno(errno);

	for (int i = 0; i < count; ++i) {
		MACAddress address(info[i].bdaddr.b);

		logger().debug("determine name of device " + address.toString(':'),
				__FILE__, __LINE__);

		char name[248];
		::memset(name, 0, sizeof(name));

		const int ret = ::hci_read_remote_name(
			sock, &info[i].bdaddr, sizeof(name), name, 0);
		if (ret < 0)
			devices.emplace(address, string("unknown"));
		else
			devices.emplace(address, string(name));

		logger().debug("detected device "
				+ address.toString(':')
				+ " with name "
				+ string(name),
				__FILE__, __LINE__);
	}

	::bt_free(info);
	::hci_close_dev(sock);

	return devices;
}

HciInfo BluezHciInterface::info() const
{
	FdAutoClose sock(hciSocket());
	return HciInfo(findHciInfo(*sock, m_name));
}

HciConnection::Ptr BluezHciInterface::connect(
		const MACAddress&,
		const Poco::Timespan&) const
{
	throw NotImplementedException(__func__);
}

void BluezHciInterface::watch(
		const MACAddress&,
		SharedPtr<WatchCallback>)
{
	throw NotImplementedException(__func__);
}

void BluezHciInterface::unwatch(const MACAddress&)
{
	throw NotImplementedException(__func__);
}

string BluezHciInterface::parseLEName(uint8_t *eir, size_t length)
{
	size_t offset = 0;

	while (offset < length) {
		uint8_t fieldLen = eir[0];

		if (fieldLen == 0)
			break;

		if (offset + fieldLen > length)
			break;

		switch (eir[1]) {
		case EIR_NAME_SHORT:
		case EIR_NAME_COMPLETE:
			size_t nameLen = fieldLen - 1;
			return string((char*)&eir[2], nameLen);
		}

		offset += fieldLen + 1;
		eir += fieldLen + 1;
	}

	return "";
}

bool BluezHciInterface::processNextEvent(const int &fd, map<MACAddress, string> &devices) const
{
	vector<char> buf(HCI_MAX_EVENT_SIZE);

	ssize_t rlen = read(fd, buf.data(), buf.size());
	if (rlen < 0 && errno == EAGAIN)
		return true;

	if (rlen < 0) {
		if (devices.size() == 0) {
			throwFromErrno(errno, "read failed");
		}
		else {
			logger().error("read: " + Error::getMessage(errno));
			return false;
		}
	}

	if (rlen == 0)
		return false;

	if (logger().trace())
		logger().trace("read " + to_string(rlen) + " bytes", __FILE__, __LINE__);

	const evt_le_meta_event *meta = skipHciEventHdr(buf.data(), buf.size());

	if (meta->subevent != EVT_LE_ADVERTISING_REPORT) {
		logger().debug("meta->subevent != EVT_LE_ADVERTISING_REPORT (0x02)",
			__FILE__, __LINE__);
		return false;
	}

	le_advertising_info *info = (le_advertising_info *) (meta->data + 1);
	if (info->length <= 0)
		return true;

	MACAddress address(info->bdaddr.b);
	string name = parseLEName(info->data, info->length);

	auto it = devices.emplace(address, name);
	if (it.second) {
		logger().debug("found BLE device: "
			+ address.toString(':') + " " + name, __FILE__, __LINE__);
	}
	else if (devices[address].empty() && !name.empty()) {
		devices[address] = name;
		logger().debug("updated BLE device: "
			+ address.toString(':') + " " + name, __FILE__, __LINE__);
	}

	return true;
}

map<MACAddress, string> BluezHciInterface::listLE(const int sock, const Timespan &timeout) const
{
	if (timeout.totalSeconds() <= 0)
		throw InvalidArgumentException("timeout for BLE scan must be at least 1 second");

	struct hci_filter newFilter, oldFilter;
	socklen_t oldFilterLen = sizeof(oldFilter);

	if (getsockopt(sock, SOL_HCI, HCI_FILTER, &oldFilter, &oldFilterLen) < 0)
		throwFromErrno(errno, "getsockopt(HCI_FILTER)");

	hci_filter_clear(&newFilter);
	hci_filter_set_ptype(HCI_EVENT_PKT, &newFilter);
	hci_filter_set_event(EVT_LE_META_EVENT, &newFilter);

	if (setsockopt(sock, SOL_HCI, HCI_FILTER, &newFilter, sizeof(newFilter)) < 0)
		throwFromErrno(errno, "setsockopt(HCI_FILTER)");

	struct pollfd pollst;
	pollst.fd = sock;
	pollst.events = POLLIN | POLLRDNORM;

	Clock start;
	map<MACAddress, string> devices;

	while (1) {
		const Timespan timeDiff = timeout - start.elapsed();
		if (timeDiff <= 0) {
			logger().debug("timeout occured while listing BLE", __FILE__, __LINE__);
			break;
		}

		auto ret = ::poll(&pollst, 1, timeDiff.totalSeconds() * 1000);
		if (ret < 0) {
			if (devices.size() == 0) {
				throwFromErrno(errno, "poll failed");
			}
			else {
				logger().error("poll: " + Error::getMessage(errno));
				break;
			}
		}
		else if (ret == 0) {
			logger().debug("BLE read timeout");
			break;
		}

		if (!processNextEvent(pollst.fd, devices)) {
			break;
		}
	}

	setsockopt(sock, SOL_HCI, HCI_FILTER, &oldFilter, sizeof(oldFilter));
	return devices;
}


map<MACAddress, string> BluezHciInterface::lescan(const Timespan &seconds) const
{
	const auto dev = findHci(m_name);
	HciAutoClose sock(::hci_open_dev(dev));

	if (*sock < 0)
		throwFromErrno(errno, "BLE hci_open_dev(" + m_name + ")");

	if (::hci_le_set_scan_parameters(*sock, LE_TYPE, htobs(LE_INTERVAL),
			 htobs(LE_WINDOW), LE_OWN_TYPE, LE_FILTER, LE_TO) < 0)
		throwFromErrno(errno, "BLE cannot set parameters for scan");

	if (::hci_le_set_scan_enable(*sock, LE_ENABLE, LE_FILTER_DUP, LE_TO) < 0)
		throwFromErrno(errno, "BLE cannot enable scan");

	map<MACAddress, string> devices;

	logger().information("starting BLE scan for "
		+ to_string(seconds.totalSeconds()) + " seconds", __FILE__, __LINE__);

	devices = listLE(*sock, seconds);

	if (::hci_le_set_scan_enable(*sock, LE_DISABLE, LE_FILTER_DUP, LE_TO) < 0)
		throwFromErrno(errno, "failed disabling BLE scan parameters");

	logger().information("BLE scan has finished, found "
		+ to_string(devices.size()) + " devices", __FILE__, __LINE__);

	return devices;
}

HciInterface::Ptr BluezHciInterfaceManager::lookup(const string &name)
{
	return new BluezHciInterface(name);
}
