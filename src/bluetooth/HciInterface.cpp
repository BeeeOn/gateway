#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "bluetooth/HciInterface.h"
#include "io/AutoClose.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const int INQUIRY_LENGTH = 8; // ~10 seconds
static const int MAX_RESPONSES = 255;

HciInterface::HciInterface(const std::string &name) :
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

int HciInterface::hciSocket() const
{
	int sock = ::socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (sock < 0)
		throwFromErrno(errno);

	return sock;
}

int HciInterface::findHci(const std::string &name) const
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

int HciInterface::findHci(int sock, const std::string &name) const
{
	struct hci_dev_info info = findHciInfo(sock, name);
	return info.dev_id;
}

void HciInterface::up() const
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

bool HciInterface::detect(const MACAddress &address) const
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

map<MACAddress, string> HciInterface::scan() const
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

HciInfo HciInterface::info() const
{
	FdAutoClose sock(hciSocket());
	return HciInfo(findHciInfo(*sock, m_name));
}
