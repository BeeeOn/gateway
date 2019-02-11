#include <Poco/Logger.h>

#include "di/Injectable.h"
#include "iqrf/DPAIQHomeProtocol.h"
#include "iqrf/DPARequest.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

BEEEON_OBJECT_BEGIN(BeeeOn, DPAIQHomeProtocol)
BEEEON_OBJECT_CASTABLE(DPAProtocol)
BEEEON_OBJECT_PROPERTY("typesMapping", &DPAIQHomeProtocol::loadTypesMapping)
BEEEON_OBJECT_END(BeeeOn, DPAIQHomeProtocol)

static const string IQ_HOME_VENDOR_NAME = "IQHome";
static const uint16_t IQ_HOME_HWPID = 0x15AF;
static const size_t IQ_HOME_PRODUCT_INFO_SIZE = 16;
static const size_t IQ_HOME_PRODUCT_INFO_CODE_SIZE = 11;

DPAIQHomeProtocol::DPAIQHomeProtocol():
	DPAMappedProtocol("iqrf-iqhome-mapping", "iqrf-iqhome")
{
}

DPARequest::Ptr DPAIQHomeProtocol::dpaModulesRequest(
		DPAMessage::NetworkAddress address) const
{
	DPARequest::Ptr request = new DPARequest;

	request->setNetworkAddress(address);
	request->setPeripheralNumber(0x30);
	request->setPeripheralCommand(0x00);
	request->setHWPID(0xffff);

	return request;
}

list<ModuleType> DPAIQHomeProtocol::extractModules(
		const vector<uint8_t> &message) const
{
	vector <uint8_t> newMessage;
	for (size_t i = 1; i < message.size(); i += 3)
		newMessage.push_back(message.at(i));

	return DPAMappedProtocol::extractModules(newMessage);
}

DPARequest::Ptr DPAIQHomeProtocol::dpaValueRequest(
		DPAMessage::NetworkAddress node,
		const list<ModuleType> &) const
{
	return dpaModulesRequest(node);
}

SensorData DPAIQHomeProtocol::parseValue(
		const list<ModuleType> &modules,
		const vector<uint8_t> &msg) const
{
	if (msg.empty())
		throw ProtocolException("response with measured values is empty");

	return DPAMappedProtocol::parseValue(modules, {begin(msg) + 1, end(msg)});
}

DPARequest::Ptr DPAIQHomeProtocol::pingRequest(
		DPAMessage::NetworkAddress node) const
{
	DPARequest::Ptr request = new DPARequest;

	request->setNetworkAddress(node);
	request->setPeripheralNumber(0x3e);
	request->setPeripheralCommand(0x00);
	request->setHWPID(0xffff);

	return request;
}

SharedPtr<DPARequest> DPAIQHomeProtocol::dpaProductInfoRequest(
		DPAMessage::NetworkAddress address) const
{
	DPARequest::Ptr request = new DPARequest;

	request->setNetworkAddress(address);
	request->setPeripheralNumber(0x3e);
	request->setPeripheralCommand(0x00);
	request->setHWPID(0xffff);

	return request;
}

DPAProtocol::ProductInfo DPAIQHomeProtocol::extractProductInfo(
		const vector<uint8_t> &msg,
		uint16_t hwPID) const
{
	if (msg.size() != IQ_HOME_PRODUCT_INFO_SIZE) {
		throw ProtocolException(
			to_string(msg.size())
			+ " is invalid size of product info response");
	}

	if (hwPID != IQ_HOME_HWPID) {
		throw InvalidArgumentException(
			"invalid IQ Home HWPID");
	}

	string productName(msg.begin(), msg.begin() + IQ_HOME_PRODUCT_INFO_CODE_SIZE);

	return {IQ_HOME_VENDOR_NAME, trimInPlace(productName)};
}
