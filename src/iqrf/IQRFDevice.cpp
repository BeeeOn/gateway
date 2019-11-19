#include <Poco/Clock.h>
#include <Poco/Logger.h>

#include "iqrf/DPAProtocol.h"
#include "iqrf/DPAResponse.h"
#include "iqrf/IQRFDevice.h"
#include "iqrf/IQRFUtil.h"
#include "iqrf/request/DPAOSPeripheralInfoRequest.h"
#include "iqrf/response/DPAOSPeripheralInfoResponse.h"
#include "util/ClassInfo.h"
#include "IQRFDevice.h"


using namespace BeeeOn;
using namespace Poco;
using namespace std;

IQRFDevice::IQRFDevice(
		IQRFMqttConnector::Ptr connector,
		const Timespan &receiveTimeout,
		DPAMessage::NetworkAddress address,
		DPAProtocol::Ptr protocol,
		const RefreshTime &refreshTime,
		const RefreshTime &refreshTimePeripheralInfo,
		IQRFEventFirer::Ptr eventFirer):
	m_connector(connector),
	m_receiveTimeout(receiveTimeout),
	m_address(address),
	m_protocol(protocol),
	m_refreshTime(refreshTime),
	m_refreshTimePeripheralInfo(refreshTimePeripheralInfo),
	m_remainingValueTime(0),
	m_remainingPeripheralInfoTime(0),
	m_remaining(1 * Timespan::SECONDS),
	m_eventFirer(eventFirer)
{
}

DPAMessage::NetworkAddress IQRFDevice::networkAddress() const
{
	return m_address;
}

uint32_t IQRFDevice::mid() const
{
	return m_mid;
}

list<ModuleType> IQRFDevice::modules() const
{
	return m_modules;
}

DPAProtocol::Ptr IQRFDevice::protocol() const
{
	return m_protocol;
}

uint16_t IQRFDevice::HWPID() const
{
	return m_HWPID;
}

void IQRFDevice::setHWPID(uint16_t HWPID)
{
	m_HWPID = HWPID;
}

DeviceID IQRFDevice::id() const
{
	uint64_t id = 0;
	id = uint64_t(m_mid) << 16;
	id |= m_HWPID;

	return DeviceID(DevicePrefix::PREFIX_IQRF, id);
}

string IQRFDevice::vendorName() const
{
	return m_vendorName;
}

string IQRFDevice::productName() const
{
	return m_productName;
}

ModuleID IQRFDevice::batteryModuleID() const
{
	return m_modules.size() - 2;
}

ModuleID IQRFDevice::rssiModuleID() const
{
	return m_modules.size() - 1;
}

string IQRFDevice::toString() const
{
	string repr;

	repr += "deviceID: ";
	repr += id().toString();
	repr += "; ";

	repr += "node: ";
	repr += to_string(m_address);
	repr += "; ";

	repr += "protocol: ";
	repr += (m_protocol) ?
		ClassInfo::forPointer(m_protocol.get()).name() : "none";
	repr += "; ";

	repr += "vendor: ";
	repr += m_vendorName;
	repr += "; ";

	repr += "product: ";
	repr += m_productName;
	repr += "; ";

	repr += "mid: ";
	repr += NumberFormatter::formatHex(m_mid, true);

	if (!m_modules.empty()) {
		repr += "; ";
		repr += "modules: ";

		for (const auto &module : m_modules) {
			repr += module.type().toString();
			repr += ", ";
		}

		repr.pop_back();
		repr.pop_back();
	}

	return repr;
}

void IQRFDevice::probe(
	const Timespan &methodTimeout)
{
	const Clock started;

	m_HWPID = detectNodeHWPID(methodTimeout - started.elapsed());
	m_mid = detectMID(methodTimeout - started.elapsed());
	m_modules = detectModules(methodTimeout - started.elapsed());

	const DPAProtocol::ProductInfo productInfo =
		detectProductInfo(methodTimeout - started.elapsed());
	m_vendorName = productInfo.vendorName;
	m_productName = productInfo.productName;
}

uint16_t IQRFDevice::detectNodeHWPID(
		const Timespan &methodTimeout)
{
	if (logger().trace())
		logger().trace("detect of HWPID", __FILE__, __LINE__);

	if (m_receiveTimeout > methodTimeout) {
		throw TimeoutException(
			"received timeout is less than maximum timeout of method");
	}

	m_eventFirer->fireDPAStatistics(m_protocol->pingRequest(m_address));

	const IQRFJsonResponse::Ptr response =
		IQRFUtil::makeRequest(
			m_connector,
			m_protocol->pingRequest(m_address),
			m_receiveTimeout
		);

	m_eventFirer->fireDPAStatistics(DPAResponse::fromRaw(response->response()));

	return DPAResponse::fromRaw(response->response())->HWPID();
}

uint32_t IQRFDevice::detectMID(const Timespan &methodTimeout)
{
	if (logger().trace())
		logger().trace("detect of MID", __FILE__, __LINE__);

	if (m_receiveTimeout > methodTimeout) {
		throw TimeoutException(
			"received timeout is less than maximum timeout of method");
	}

	DPARequest::Ptr dpaPeripheralInfo = new DPAOSPeripheralInfoRequest(m_address);
	m_eventFirer->fireDPAStatistics(dpaPeripheralInfo);

	const IQRFJsonResponse::Ptr response =
		IQRFUtil::makeRequest(
			m_connector,
			dpaPeripheralInfo,
			m_receiveTimeout
		);

	const DPAResponse::Ptr dpa = DPAResponse::fromRaw(response->response());

	m_eventFirer->fireDPAStatistics(dpa);

	return dpa.cast<DPAOSPeripheralInfoResponse>()->mid();
}

list<ModuleType> IQRFDevice::detectModules(const Timespan &methodTimeout)
{
	if (logger().trace()) {
		logger().trace(
			"detect of modules for node "
			+ NumberFormatter::formatHex(m_address, true),
			__FILE__, __LINE__);
	}

	if (m_receiveTimeout > methodTimeout) {
		throw TimeoutException(
			"received timeout is less than maximum timeout of method");
	}

	m_eventFirer->fireDPAStatistics(m_protocol->dpaModulesRequest(m_address));

	const IQRFJsonResponse::Ptr response =
		IQRFUtil::makeRequest(
			m_connector,
			m_protocol->dpaModulesRequest(m_address),
			m_receiveTimeout
		);

	m_eventFirer->fireDPAStatistics(DPAResponse::fromRaw(response->response()));

	return m_protocol->extractModules(
		DPAResponse::fromRaw(response->response())
			->peripheralData()
	);
}

DPAProtocol::ProductInfo IQRFDevice::detectProductInfo(
		const Timespan &methodTimeout)
{
	if (logger().trace()) {
		logger().trace(
			"detect of product info for node "
			+ NumberFormatter::formatHex(m_address, true),
			__FILE__, __LINE__);
	}

	if (m_receiveTimeout > methodTimeout) {
		throw TimeoutException(
			"received timeout is less than maximum timeout of method");
	}

	m_eventFirer->fireDPAStatistics(m_protocol->dpaProductInfoRequest(m_address));

	const IQRFJsonResponse::Ptr response =
		IQRFUtil::makeRequest(
			m_connector,
			m_protocol->dpaProductInfoRequest(m_address),
			m_receiveTimeout
		);

	const DPAResponse::Ptr dpa = DPAResponse::fromRaw(response->response());

	m_eventFirer->fireDPAStatistics(dpa);

	return m_protocol->extractProductInfo(dpa->peripheralData(), m_HWPID);
}

SensorData IQRFDevice::obtainValues()
{
	DPARequest::Ptr dpaValueRequest = m_protocol->dpaValueRequest(m_address, m_modules);

	m_eventFirer->fireDPAStatistics(dpaValueRequest);

	const IQRFJsonResponse::Ptr valueResponse =
		IQRFUtil::makeRequest(
			m_connector,
			dpaValueRequest,
			m_receiveTimeout
		);
	DPAResponse::Ptr dpaValue =
		DPAResponse::fromRaw(valueResponse->response());

	m_eventFirer->fireDPAStatistics(dpaValue);

	SensorData sensorData =
		m_protocol->parseValue(
			m_modules,
			dpaValue->peripheralData()
		);
	sensorData.setDeviceID(id());

	return sensorData;
}

SensorData IQRFDevice::obtainPeripheralInfo()
{
	ModuleID batteryID = batteryModuleID();
	ModuleID rssiID = rssiModuleID();

	DPARequest::Ptr dpaPeripheralInfo = new DPAOSPeripheralInfoRequest(m_address);
	m_eventFirer->fireDPAStatistics(dpaPeripheralInfo);

	IQRFJsonResponse::Ptr peripheralResponse =
		IQRFUtil::makeRequest(
			m_connector,
			dpaPeripheralInfo,
			m_receiveTimeout
		);
	DPAOSPeripheralInfoResponse::Ptr dpaPeripheral =
		DPAResponse::fromRaw(peripheralResponse->response()).cast<DPAOSPeripheralInfoResponse>();

	m_eventFirer->fireDPAStatistics(static_cast<DPAResponse::Ptr>(dpaPeripheral));

	SensorData sensorData;
	sensorData.setDeviceID(id());
	sensorData.insertValue({batteryID, dpaPeripheral->percentageSupplyVoltage()});
	sensorData.insertValue({rssiID, dpaPeripheral->rssiPercentage()});

	return sensorData;
}

RefreshTime IQRFDevice::refresh() const
{
	return RefreshTime::fromSeconds(m_remaining.totalSeconds());
}

void IQRFDevice::poll(Distributor::Ptr distributor)
{
	if (m_remainingValueTime.totalSeconds() <= 0) {
		m_remainingValueTime = m_refreshTime;
		distributor->exportData(obtainValues());
	}

	if (m_remainingPeripheralInfoTime.totalSeconds() <= 0) {
		m_remainingPeripheralInfoTime = m_refreshTimePeripheralInfo;
		distributor->exportData(obtainPeripheralInfo());
	}

	Timespan remainingTime;
	if (m_remainingValueTime < m_remainingPeripheralInfoTime)
		remainingTime = m_remainingValueTime;
	else
		remainingTime = m_remainingPeripheralInfoTime;

	m_remainingValueTime -= remainingTime;
	m_remainingPeripheralInfoTime -= remainingTime;
	m_remaining = remainingTime;
}
