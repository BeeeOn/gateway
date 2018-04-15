 #include <Poco/NumberParser.h>

#include "iqrf/IQRFJsonMessage.h"
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

IQRFJsonMessage::Ptr IQRFJsonMessage::parse(const string &data)
{
	JSON::Object::Ptr json = JsonUtil::parse(data);

	string id;
	string request;
	Timespan timeout;

	if (!json->has("msgid"))
		InvalidArgumentException("missing msgid attribute");

	if (!json->has("timeout"))
		InvalidArgumentException("missing timeout attribute");

	if (!json->has("request"))
		InvalidArgumentException("missing request attribute");

	id = json->getValue<string>("msgid");
	timeout = NumberParser::parseUnsigned(
		json->getValue<string>("timeout")) * Timespan::MILLISECONDS;
	request = json->getValue<string>("request");
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
