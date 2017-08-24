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
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/RegularExpression.h>
#include <Poco/StreamCopier.h>
#include <Poco/String.h>
#include <Poco/StringTokenizer.h>

#include "bluetooth/HciInterface.h"
#include "io/AutoClose.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

HciInterface::HciInterface(const std::string &name) :
	m_name(name)
{
}

/**
 * Convert the given errno value into the appropriate exception
 * and throw it.
 */
static void throwFromErrno(const int e)
{
	throw IOException(::strerror(e));
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

list<pair<string, MACAddress>> HciInterface::scan() const
{
	string result;
	vector<string> args;
	args.push_back("-i");
	args.push_back(m_name);
	args.push_back("scan");

	int code;
	if ((code = exec("hcitool", args, result)))
		throw IOException("hcitool failed to perform scan: " + to_string(code));

	return parseScan(result);
}

int HciInterface::exec(const string &command, const vector<string> &args, string &output) const
{
	Mutex::ScopedLock lock(m_mutexExec);

	Pipe outPipe;
	ProcessHandle ph = Process::launch(command, args, NULL, &outPipe, NULL);
	PipeInputStream istr(outPipe);
	StreamCopier::copyToString(istr, output);

	Process::requestTermination(ph.id());
	return ph.wait();
}

list<pair<string, MACAddress>> HciInterface::parseScan(const string &scan) const
{
	const RegularExpression expr("^(([0-9A-Fa-f]{2}[:-]){5}[0-9A-Fa-f]{2})\t((.)+)$");

	list<pair<string, MACAddress>> scanedDevices;
	vector<string> lines = split(scan, "\n");

	for (auto line: lines) {
		RegularExpression::MatchVec posVec;
		if (!expr.match(line, 0, posVec))
			continue;

		const string macAddress = line.substr(posVec[1].offset, posVec[1].length);
		const string deviceName = line.substr(posVec[3].offset, posVec[3].length);

		scanedDevices.push_back(make_pair(deviceName, MACAddress::parse(macAddress)));
	}
	return scanedDevices;
}

vector<string> HciInterface::split(const string &input, const string &div) const
{
	vector<string> output;

	StringTokenizer st(input, div,
		StringTokenizer::TOK_IGNORE_EMPTY | StringTokenizer::TOK_TRIM);

	for (auto item: st)
		output.push_back(item);

	return output;
}
