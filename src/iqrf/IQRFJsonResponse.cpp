#include "iqrf/IQRFJsonResponse.h"

#ifndef JSON_PRESERVE_KEY_ORDER
#define JSON_PRESERVE_KEY_ORDER 1
#endif

using namespace BeeeOn;
using namespace std;
using namespace Poco;

IQRFJsonResponse::IQRFJsonResponse() :
	IQRFJsonRequest(),
	m_errorCode(DpaError::STATUS_NO_ERROR)
{
}

string IQRFJsonResponse::response() const
{
	return m_rawData.response;
}

void IQRFJsonResponse::setResponse(const string &response)
{
	m_rawData.response = response;
}

void IQRFJsonResponse::setStatus(const string& statString, int statNum)
{
	m_statusStr = statString;
	m_status = statNum;
	m_errorCode = static_cast<IQRFJsonResponse::DpaErrorEnum::Raw>(statNum);
}

void IQRFJsonResponse::setGWIdentification(const string& identification)
{
	m_insId = identification;
}

void IQRFJsonResponse::setRawData(const RawData &raw)
{
	m_rawData = raw;
}

string IQRFJsonResponse::request() const
{
	return m_rawData.request;
}

void IQRFJsonResponse::setRequest(const string &request)
{
	m_rawData.request = request;
}

string IQRFJsonResponse::toString()
{
	JSON::Object::Ptr root = new JSON::Object(JSON_PRESERVE_KEY_ORDER);
	JSON::Object::Ptr data = new JSON::Object(JSON_PRESERVE_KEY_ORDER);
	JSON::Object::Ptr response = new JSON::Object(JSON_PRESERVE_KEY_ORDER);

	Poco::JSON::Array::Ptr raw = new Poco::JSON::Array();

	root->set("mType", "iqrfRaw");

	data->set("msgId", messageID());
	data->set("timeout", static_cast<int>(timeout().totalMilliseconds()));

	// DPA datagram
	response->set("rData", m_rawData.response);
	data->set("rsp", response);

	root->set("data", data);

	JSON::Object::Ptr raw_request = new JSON::Object(JSON_PRESERVE_KEY_ORDER);
	JSON::Object::Ptr raw_request_ts = new JSON::Object(JSON_PRESERVE_KEY_ORDER);
	JSON::Object::Ptr raw_confirmation = new JSON::Object(JSON_PRESERVE_KEY_ORDER);
	JSON::Object::Ptr raw_confirmation_ts = new JSON::Object(JSON_PRESERVE_KEY_ORDER);
	JSON::Object::Ptr raw_response = new JSON::Object(JSON_PRESERVE_KEY_ORDER);
	JSON::Object::Ptr raw_response_ts = new JSON::Object(JSON_PRESERVE_KEY_ORDER);

	raw_request->set("request", m_rawData.request);
	raw_request->set("requestTs", m_rawData.requestTs);
	raw_request->set("confirmation", m_rawData.confirmation);
	raw_request->set("confirmationTS", m_rawData.confirmationTs);
	raw_request->set("response", m_rawData.response);
	raw_request->set("responseTs", m_rawData.responseTs);

	raw->set(0,raw_request);
	raw->set(1,raw_request_ts);
	raw->set(2,raw_confirmation);
	raw->set(3,raw_confirmation_ts);
	raw->set(4,raw_response);
	raw->set(5,raw_response_ts);

	data->set("raw", raw_request);

	data->set("insId", m_insId);
	data->set("status", m_status);
	data->set("statusStr", m_statusStr);

	return Dynamic::Var::toString(root);
}

IQRFJsonResponse::DpaError IQRFJsonResponse::errorCode() const
{
	return m_errorCode;
}

void IQRFJsonResponse::setErrorCode(
	const IQRFJsonResponse::DpaError &errCode)
{
	m_errorCode = errCode;
	m_status = static_cast<int>(errCode);
}

EnumHelper<IQRFJsonResponse::DpaErrorEnum::Raw>::ValueMap
	&IQRFJsonResponse::DpaErrorEnum::valueMap()
{
	static EnumHelper<DpaErrorEnum::Raw>::ValueMap valueMap = {
		{STATUS_NO_ERROR, "STATUS_NO_ERROR"},
		{ERROR_FAIL, "ERROR_FAIL"},
		{ERROR_PCMD, "ERROR_PCMD"},
		{ERROR_PNUM, "ERROR_PNUM"},
		{ERROR_ADDR, "ERROR_ADDR"},
		{ERROR_DATA_LEN, "ERROR_DATA_LEN"},
		{ERROR_DATA, "ERROR_DATA"},
		{ERROR_HWPID, "ERROR_HWPID"},
		{ERROR_NADR, "ERROR_NADR"},
		{ERROR_IFACE_CUSTOM_HANDLER, "ERROR_IFACE_CUSTOM_HANDLER"},
		{ERROR_MISSING_CUSTOM_DPA_HANDLER, "ERROR_MISSING_CUSTOM_DPA_HANDLER"},
		{ERROR_TIMEOUT, "ERROR_TIMEOUT"},
		{STATUS_CONFIRMATION, "STATUS_CONFIRMATION"}
	};

	return valueMap;
}
