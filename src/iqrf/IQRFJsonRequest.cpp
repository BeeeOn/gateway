#include "iqrf/IQRFJsonRequest.h"

#ifndef JSON_PRESERVE_KEY_ORDER
#define JSON_PRESERVE_KEY_ORDER 1
#endif

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
	JSON::Object::Ptr root = new JSON::Object(JSON_PRESERVE_KEY_ORDER);
	JSON::Object::Ptr data = new JSON::Object(JSON_PRESERVE_KEY_ORDER);
	JSON::Object::Ptr request = new JSON::Object(JSON_PRESERVE_KEY_ORDER);

	root->set("mType", "iqrfRaw");

	data->set("msgId", messageID());
	data->set("timeout", static_cast<int>(timeout().totalMilliseconds()));

	// DPA datagram
	request->set("rData", m_request);
	data->set("req",request);

	data->set("returnVerbose", true);

	root->set("data", data);

	return Dynamic::Var::toString(root);
}
