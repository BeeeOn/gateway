#include <Poco/Clock.h>
#include <Poco/DateTimeParser.h>
#include <Poco/Logger.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>
#include <Poco/StringTokenizer.h>
#include "Poco/Timezone.h"
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>

#include "conrad/FHEMClient.h"
#include "di/Injectable.h"
#include "util/JsonUtil.h"

#define MAX_BUFFER_SIZE 1024

BEEEON_OBJECT_BEGIN(BeeeOn, FHEMClient)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_PROPERTY("address", &FHEMClient::setFHEMAddress)
BEEEON_OBJECT_PROPERTY("refreshTime", &FHEMClient::setRefreshTime)
BEEEON_OBJECT_PROPERTY("receiveTimeout", &FHEMClient::setReceiveTimeout)
BEEEON_OBJECT_PROPERTY("reconnectTime", &FHEMClient::setReconnectTime)
BEEEON_OBJECT_END(BeeeOn, FHEMClient)

using namespace std;
using namespace BeeeOn;
using namespace Poco;
using namespace Poco::JSON;
using namespace Poco::Net;

FHEMClient::FHEMClient():
	m_refreshTime(5 * Timespan::SECONDS),
	m_receiveTimeout(2 * Timespan::SECONDS),
	m_reconnectTime(5 * Timespan::SECONDS),
	m_fhemAddress("127.0.0.1:7072")
{
}

void FHEMClient::setRefreshTime(const Timespan &time)
{
	if (time.totalSeconds() <= 0)
		throw InvalidArgumentException("refresh time must be at least a second");

	m_refreshTime = time;
}

void FHEMClient::setReceiveTimeout(const Timespan &timeout)
{
	if (timeout.totalSeconds() <= 0)
		throw InvalidArgumentException("receive timeout must be at least a second");

	m_receiveTimeout = timeout;
}

void FHEMClient::setReconnectTime(const Timespan &time)
{
	if (time.totalSeconds() <= 0)
		throw InvalidArgumentException("reconnect time must be at least a second");

	m_reconnectTime = time;
}

void FHEMClient::setFHEMAddress(const string &address)
{
	m_fhemAddress = SocketAddress(address);
}

void FHEMClient::run()
{
	logger().information("starting FHEM client", __FILE__, __LINE__);

	StopControl::Run run(m_stopControl);

	while (run) {
		try {
			initConnection();
			break;
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
		}

		run.waitStoppable(m_reconnectTime);
	}

	while (run) {
		try {
			cycle();
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
		}

		run.waitStoppable(m_refreshTime);
	}

	logger().information("stopping FHEM client", __FILE__, __LINE__);
}

void FHEMClient::stop()
{
	m_stopControl.requestStop();
	m_receiveEvent.set();
}

void FHEMClient::sendRequest(const string& request)
{
	DialogSocket tnSocket(m_fhemAddress);
	tnSocket.setReceiveTimeout(m_receiveTimeout);

	tnSocket.sendMessage(request);
}

Object::Ptr FHEMClient::receive(const Timespan &timeout)
{
	const Poco::Clock now;

	while (!m_stopControl.shouldStop()) {
		if (now.isElapsed(timeout.totalMicroseconds()) && timeout > 0)
			throw TimeoutException("receive timeout expired");

		Object::Ptr event = nextEvent();
		if (!event.isNull())
			return event;

		if (timeout < 0) {
			m_receiveEvent.wait();
		}
		else {
			Timespan waitTime = timeout.totalMicroseconds() - now.elapsed();

			if (waitTime <= 0)
				throw TimeoutException("receive timeout expired");

			if (waitTime.totalMilliseconds() < 1)
				waitTime = 1 * Timespan::MILLISECONDS;

			m_receiveEvent.wait(waitTime.totalMilliseconds());
		}
	}

	return {};
}

void FHEMClient::initConnection()
{
	m_telnetSocket = DialogSocket(m_fhemAddress);
	m_telnetSocket.setReceiveTimeout(m_receiveTimeout);
}

void FHEMClient::cycle()
{
	vector<string> devices;
	retrieveHomeMaticDevices(devices);

	for (auto device : devices) {
		try {
			processDevice(device);
		}
		catch (const Exception& e) {
			logger().warning("processing of device " + device + " failed",
				__FILE__, __LINE__);
			logger().log(e, __FILE__, __LINE__);
		}
	}
}

Object::Ptr FHEMClient::nextEvent()
{
	FastMutex::ScopedLock guard(m_queueMutex);
	Object::Ptr event;

	if (m_eventsQueue.empty())
		return event;

	event = m_eventsQueue.front();
	m_eventsQueue.pop();

	return event;
}

void FHEMClient::retrieveHomeMaticDevices(vector<string>& devices)
{
	Object::Ptr jsonMsg = sendCommand("jsonlist2 ActionDetector");
	Array::Ptr results = jsonMsg->getArray("Results");
	if (results->size() <= 0)
		return;

	Object::Ptr actionDetector = results->getObject(0);
	Object::Ptr readings = actionDetector->getObject("Readings");

	RegularExpression reDevice("status_(HM_[a-zA-Z0-9]+)");
	Object::Iterator it;
	for (it = readings->begin(); it != readings->end(); it++) {
		RegularExpression::MatchVec matches;
		if (reDevice.match(it->first, 0, matches) == 0)
			continue;

		string device = it->first.substr(matches[1].offset, matches[1].length);
		devices.push_back(device);
	}
}

void FHEMClient::processDevice(const string& device)
{
	Object::Ptr jsonMsg = sendCommand("jsonlist2 " + device);
	Array::Ptr results = jsonMsg->getArray("Results");
	Object::Ptr deviceElement = results->getObject(0);
	Object::Ptr internals = deviceElement->getObject("Internals");
	Object::Ptr attributes = deviceElement->getObject("Attributes");

	FHEMDeviceInfo deviceInfo = assembleDeviceInfo(device, internals);

	string type = attributes->getValue<string>("subType");
	string model = attributes->getValue<string>("model");
	string serialNumber =  attributes->getValue<string>("serialNr");

	auto itInfo = m_deviceInfos.emplace(device, deviceInfo);
	// new device event
	if (itInfo.second) {
		createNewDeviceEvent(device, model, type, serialNumber);

		logger().information("generate new_device event for device " + device,
			__FILE__, __LINE__);

		return;
	}

	// statistic event
	if (itInfo.first->second.protRcv() < deviceInfo.protRcv()) {
		itInfo.first->second.setProtRcv(deviceInfo.protRcv());

		createStatEvent("rcv_cnt", device);

		logger().information("generate rcv_cnt event for device " + device,
			__FILE__, __LINE__);
	}

	// statistic event
	if (itInfo.first->second.protSnd() < deviceInfo.protSnd()) {
		itInfo.first->second.setProtSnd(deviceInfo.protSnd());

		createStatEvent("snd_cnt", device);

		logger().information("generate snd_cnt event for device " + device,
			__FILE__, __LINE__);
	}

	// message event
	if (itInfo.first->second.lastRcv() < deviceInfo.lastRcv()) {
		itInfo.first->second.setLastRcv(deviceInfo.lastRcv());

		string rawMsg = internals->getValue<string>("CUL_0_RAWMSG");
		rawMsg = rawMsg.substr(0, rawMsg.find(":"));

		string rssiStr = internals->getValue<string>("CUL_0_RSSI");
		double rssi = NumberParser::parseFloat(rssiStr);

		map<string, string> channels;
		retrieveChannelsState(internals, channels);

		createMessageEvent(
			device, model, type, serialNumber, rawMsg, rssi, channels);

		logger().information("generate message event for device " + device,
			__FILE__, __LINE__);
	}
}

FHEMDeviceInfo FHEMClient::assembleDeviceInfo(
		const string& device,
		const Object::Ptr internals) const
{
	string lastRcvStr = internals->getValue<string>("protLastRcv");
	int timezonediff;
	Timestamp lastRcv = DateTimeParser::parse(
		"%Y-%m-%d %H:%M:%S", lastRcvStr, timezonediff).timestamp();

	string protRcvStr = internals->getValue<string>("protRcv");
	protRcvStr = protRcvStr.substr(0, protRcvStr.find(" "));
	uint32_t protRcv = 0;
	if (!protRcvStr.empty())
		protRcv = NumberParser::parse(protRcvStr);

	uint32_t protSnd = 0;
	if (internals->has("protSnd")) {
		string protSndStr = internals->getValue<string>("protSnd");
		protSndStr = protSndStr.substr(0, protSndStr.find(" "));
		if (!protSndStr.empty())
			protSnd = NumberParser::parse(protSndStr);
	}

	FHEMDeviceInfo deviceInfo = FHEMDeviceInfo(device, protRcv, protSnd, lastRcv);

	return deviceInfo;
}

void FHEMClient::retrieveChannelsState(
		Object::Ptr internals,
		map<string, string>& channels)
{
	channels.emplace("Main", internals->getValue<string>("STATE"));

	RegularExpression reChannel("channel_[0-9]+");
	Object::Iterator it;
	for (it = internals->begin(); it != internals->end(); it++) {
		if (reChannel.match(it->first) == 0)
			continue;

		string channelFull = internals->getValue<string>(it->first);
		StringTokenizer tokenizer(channelFull, "_");
		string channel = tokenizer[tokenizer.count() - 1];
		string channelState = retrieveChannelState(channelFull);

		channels.emplace(channel, channelState);
	}
}

string FHEMClient::retrieveChannelState(const string& channel)
{
	Object::Ptr jsonMsg = sendCommand("jsonlist2 " + channel);
	Array::Ptr results = jsonMsg->getArray("Results");
	Object::Ptr deviceElement = results->getObject(0);
	Object::Ptr internals = deviceElement->getObject("Internals");

	string state = internals->getValue<string>("STATE");

	return state;
}

void FHEMClient::createNewDeviceEvent(
		const string& device,
		const string& model,
		const string& type,
		const string& serialNumber)
{
	Object::Ptr newDeviceEvent = new Object;
	newDeviceEvent->set("event", "new_device");
	newDeviceEvent->set("dev", device);
	newDeviceEvent->set("model", model);
	newDeviceEvent->set("type", type);
	newDeviceEvent->set("serial", serialNumber);

	appendEventToQueue(newDeviceEvent);
}

void FHEMClient::createStatEvent(
		const std::string& event,
		const std::string& device)
{
	Object::Ptr statEvent = new Object;
	statEvent->set("event", event);
	statEvent->set("dev", device);

	appendEventToQueue(statEvent);
}

void FHEMClient::createMessageEvent(
		const string& device,
		const string& model,
		const string& type,
		const string& serialNumber,
		const string& rawMsg,
		const double rssi,
		const map<string, string>& channels)
{
	Object::Ptr channelsJson = new Object;
	for (auto one : channels)
		channelsJson->set(one.first, one.second);

	Object::Ptr messageEvent = new Object;
	messageEvent->set("event", "message");
	messageEvent->set("dev", device);
	messageEvent->set("model", model);
	messageEvent->set("type", type);
	messageEvent->set("serial", serialNumber);
	messageEvent->set("raw", rawMsg);
	messageEvent->set("rssi", rssi);
	messageEvent->set("channels", channelsJson);

	appendEventToQueue(messageEvent);
}

void FHEMClient::appendEventToQueue(const Object::Ptr event)
{
	FastMutex::ScopedLock guard(m_queueMutex);

	m_eventsQueue.push(event);
	m_receiveEvent.set();
}

Object::Ptr FHEMClient::sendCommand(const string& command)
{
	m_telnetSocket.sendMessage(command);
	vector<char> msg(MAX_BUFFER_SIZE);
	string completeMsg;
	size_t retChars = 0;

	do {
		retChars = m_telnetSocket.receiveRawBytes(&msg[0], MAX_BUFFER_SIZE);
		for (size_t i = 0; i < retChars; i++)
			completeMsg += msg[i];
	}
	while (retChars >= MAX_BUFFER_SIZE);

	Object::Ptr jsonMsg = JsonUtil::parse(completeMsg);

	return jsonMsg;
}
