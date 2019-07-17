 #include <Poco/NumberParser.h>

#include "iqrf/IQRFJsonMessage.h"
#include "iqrf/IQRFJsonRequest.h"
#include "iqrf/IQRFJsonResponse.h"
#include "util/JsonUtil.h"

using namespace BeeeOn;
using namespace std;
using namespace Poco;

static const Timespan DEFAULT_TIMEOUT = 2 * Timespan::SECONDS;

IQRFJsonMessage::IQRFJsonMessage():
	m_timeout(DEFAULT_TIMEOUT)
{
}

IQRFJsonMessage::~IQRFJsonMessage()
{
}

IQRFJsonMessage::Ptr IQRFJsonMessage::parse(const string &message)
{
	JSON::Object::Ptr json = JsonUtil::parse(message);

	string id;
	Timespan timeout;
	JSON::Object::Ptr data;

	if (json->has("data"))
		data = json->getObject("data");
	else
		InvalidArgumentException("missing data object");

	if (!data->has("msgId"))
		InvalidArgumentException("missing msgId attribute");

	if (!data->has("timeout"))
		InvalidArgumentException("missing timeout attribute");

	id = data->getValue<string>("msgId");

	timeout = NumberParser::parseUnsigned(
		data->getValue<string>("timeout")) * Timespan::MILLISECONDS;

	if (data->has("rsp")) {
		IQRFJsonResponse::Ptr jsonResponse = new IQRFJsonResponse;

		jsonResponse->setMessageID(id);
		jsonResponse->setTimeout(timeout);

		JSON::Object::Ptr raw_array = data->getArray("raw")->getObject(0);

		IQRFJsonResponse::RawData raw = IQRFJsonResponse::RawData();
		raw.request = raw_array->getValue<string>("request");
		raw.requestTs = raw_array->getValue<string>("requestTs");
		raw.confirmation = raw_array->getValue<string>("confirmation");
		raw.confirmationTs = raw_array->getValue<string>("confirmationTs");
		raw.response = raw_array->getValue<string>("response");
		raw.responseTs = raw_array->getValue<string>("responseTs");

		jsonResponse->setRawData(raw);

		jsonResponse->setStatus(
			data->getValue<string>("statusStr"),
			data->getValue<int>("status"));
		jsonResponse->setGWIdentification(data->getValue<string>("insId"));

		return jsonResponse;
	}
	else if (data->has("req")) {
		IQRFJsonRequest::Ptr jsonRequest = new IQRFJsonRequest;

		jsonRequest->setMessageID(id);
		jsonRequest->setTimeout(timeout);
		jsonRequest->setRequest(data->getObject("req")->getValue<string>("rData"));

		return jsonRequest;
	}

	throw InvalidArgumentException("invalid message type");
}

void IQRFJsonMessage::setTimeout(const Timespan &timeout)
{
	m_timeout = timeout;
}

Timespan IQRFJsonMessage::timeout() const
{
	return m_timeout;
}

void IQRFJsonMessage::setMessageID(const string &id)
{
	m_id = id;
}

string IQRFJsonMessage::messageID() const
{
	return m_id;
}
