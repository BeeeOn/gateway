#include "iqrf/IQRFJsonResponse.h"

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
	return m_response;
}

void IQRFJsonResponse::setResponse(const string &response)
{
	m_response = response;
}

string IQRFJsonResponse::toString()
{
	JSON::Object::Ptr json = new JSON::Object;

	json->set("ctype", "dpa");
	json->set("type", "raw");

	json->set("msgid", messageID());
	json->set("timeout", to_string(timeout().totalMilliseconds()));
	json->set("request", request());
	json->set("response", m_response);

	return Dynamic::Var::toString(json);
}

IQRFJsonResponse::DpaError IQRFJsonResponse::errorCode() const
{
	return m_errorCode;
}

void IQRFJsonResponse::setErrorCode(
	const IQRFJsonResponse::DpaError &errCode)
{
	m_errorCode = errCode;
}

EnumHelper<IQRFJsonResponse::DpaErrorEnum::Raw>::ValueMap
	&IQRFJsonResponse::DpaErrorEnum::valueMap()
{
	static EnumHelper<DpaErrorEnum::Raw>::ValueMap valueMap = {
		{ERROR_FAIL, "ERROR_FAIL"},
		{ERROR_IFACE, "ERROR_IFACE"},
		{ERROR_NADR, "ERROR_NADR"},
		{ERROR_PNUM, "ERROR_PNUM"},
		{ERROR_TIMEOUT, "ERROR_TIMEOUT"},
		{STATUS_NO_ERROR, "STATUS_NO_ERROR"},
	};

	return valueMap;
}
