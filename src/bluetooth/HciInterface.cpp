#include <cerrno>
#include <cstring>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>

#include <Poco/Exception.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/RegularExpression.h>
#include <Poco/StreamCopier.h>
#include <Poco/String.h>
#include <Poco/StringTokenizer.h>

#include "bluetooth/HciInterface.h"

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

void HciInterface::up() const
{
	string ignoredResult;
	vector<string> args;
	args.push_back(m_name);
	args.push_back("up");

	int code;
	if ((code = exec("hciconfig", args, ignoredResult))) {
		throw IOException("hciconfig " + m_name
			+ " up has failed: " + to_string(code));
	}
}

bool HciInterface::detect(const MACAddress &address) const
{
	string mac = address.toString(':');

	string result;
	vector<string> args;
	args.push_back("-i");
	args.push_back(m_name);
	args.push_back("name");
	args.push_back(mac);

	int code;
	if ((code = exec("hcitool", args, result))) {
		throw IOException("hcitool failed to scan for address "
			+ mac + ": " + to_string(code));
	}
	else {
		result = trim(result);
		return result.size() > 0;
	}
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
