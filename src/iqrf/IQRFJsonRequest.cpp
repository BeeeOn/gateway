#include "iqrf/IQRFJsonRequest.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

IQRFJsonRequest::IQRFJsonRequest():
	IQRFJsonMessage()
{
}

string IQRFJsonRequest::request() const
{
	return m_request;
}

void IQRFJsonRequest::setRequest(const string &request)
{
	m_request = request;
}

string IQRFJsonRequest::toString()
{
	JSON::Object::Ptr json = new JSON::Object;

	json->set("ctype", "dpa");
	json->set("type", "raw");

	json->set("msgid", messageID());
	json->set("timeout", static_cast<int>(timeout().totalMilliseconds()));
	json->set("request", m_request);

	json->set("response", "");

	return Dynamic::Var::toString(json);
}
