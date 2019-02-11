#include "iqrf/IQRFUtil.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

IQRFJsonResponse::Ptr IQRFUtil::makeRequest(
		IQRFMqttConnector::Ptr connector,
		DPARequest::Ptr dpa,
		const Timespan &receiveTimeout)
{
	const GlobalID messageID = GlobalID::random();

	IQRFJsonRequest::Ptr json = new IQRFJsonRequest;
	json->setRequest(dpa->toDPAString());
	json->setTimeout(receiveTimeout);
	json->setMessageID(messageID.toString());

	const auto request = json->toString();
	if (logger().trace()) {
		logger().dump(
			"sending request of size "
			+ to_string(request.size()) + " B",
			request.data(),
			request.size(),
			Message::PRIO_TRACE);
	}
	else if (logger().debug()) {
		logger().debug(
			"sending request of size "
			+ to_string(request.size()) + " B",
			__FILE__, __LINE__);
	}
	connector->send(json->toString());

	auto jsonResponse = connector->receive(messageID, receiveTimeout);
	string response = jsonResponse->toString();
	if (logger().trace()) {
		logger().dump(
			"received response of size "
			+ to_string(response.size()) + " B",
			response.data(),
			response.size(),
			Message::PRIO_TRACE);
	}
	else if (logger().debug()) {
		logger().debug(
			"received response of size "
			+ to_string(response.size()) + " B",
			__FILE__, __LINE__);
	}

	if (jsonResponse->errorCode() != IQRFJsonResponse::DpaError::STATUS_NO_ERROR)
		throw IllegalStateException(jsonResponse->errorCode().toString());

	return jsonResponse;
}

Logger &IQRFUtil::logger()
{
	return Loggable::forClass(typeid(IQRFUtil));
}
