#include <iostream>

#include <Poco/Logger.h>
#include <Poco/NumberParser.h>
#include <Poco/URI.h>
#include <Poco/RegularExpression.h>
#include <Poco/String.h>
#include <Poco/Thread.h>
#include <Poco/Crypto/Cipher.h>
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/CipherKey.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include "net/HTTPUtil.h"
#include "philips/PhilipsHueBridge.h"
#include "util/JsonUtil.h"

#define MAX_ATTEMPTS 6

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Crypto;
using namespace Poco::JSON;
using namespace Poco::Net;
using namespace std;

const Timespan PhilipsHueBridge::SLEEP_BETWEEN_ATTEMPTS = 5 * Timespan::SECONDS;

PhilipsHueBridge::PhilipsHueBridge(const SocketAddress& address):
	m_address(address),
	m_countOfBulbs(0)
{
}

PhilipsHueBridge::Ptr PhilipsHueBridge::buildDevice(const SocketAddress& address, const Timespan& timeout)
{
	PhilipsHueBridge::Ptr bridge = new PhilipsHueBridge(address);
	bridge->m_httpTimeout = timeout;
	bridge->requestDeviceInfo();

	return bridge;
}

/**
 * Example of request's body:
 * {
 *     "devicetype":"BeeeOn#gateway"
 * }
 *
 * Example of successful response's body:
 * [
 *     {
 *         "success": {
 *             "username": "YTV2PIPXrtrnHFLafGQlcVyrcxSgNo8wv-NQPmVk"
 *         }
 *     }
 * ]
 *
 * Example of failed response's body:
 * [
 *     {
 *         "error": {
 *             "type": 101,
 *             "address": "",
 *             "description": "link button not pressed"
 *         }
 *     }
 * ]
 */
string PhilipsHueBridge::authorize(const string& deviceType)
{
	ostringstream messageStream;
	Object object;
	object.set("devicetype", deviceType);
	object.stringify(messageStream);

	URI uri("/api");

	HTTPRequest request(HTTPRequest::HTTP_POST, uri.toString(), "HTTP/1.1");
	request.setContentType("application/json");
	request.setContentLength(messageStream.str().length());

	logger().information("authorization started, pressing the button on the bridge is required",
		__FILE__, __LINE__);

	for (int i = 0; i < MAX_ATTEMPTS; i++) {
		HTTPEntireResponse response = sendRequest(
			request, messageStream.str(), m_address, m_httpTimeout);

		logger().debug(response.getBody(), __FILE__, __LINE__);

		Parser jsonParser;
		Dynamic::Var var = jsonParser.parse(response.getBody());
		Array::Ptr array = var.extract<Array::Ptr>();

		Object::Ptr first = array->getObject(0);
		if (first->has("success")) {
			string username = first->getObject("success")->getValue<string>("username");

			RegularExpression re("^([a-zA-Z0-9-]+)$");
			if (re.match(username))
				return username;
			else
				throw DataFormatException("bad format of username");
		}

		Thread::sleep(SLEEP_BETWEEN_ATTEMPTS.totalMilliseconds());
	}

	throw TimeoutException("authorization failed due to expiration of timeout");
}

/**
 * Example of successful response's body:
 * [
 *     {
 *         "success": {
 *             "/lights": "Searching for new devices"
 *         }
 *     }
 * ]
 */
void PhilipsHueBridge::requestSearchNewDevices()
{
	URI uri("/api/" + username() + "/lights");

	HTTPRequest request(HTTPRequest::HTTP_POST, uri.toString(), "HTTP/1.1");
	request.setContentLength(0);

	HTTPEntireResponse response = sendRequest(request, "", m_address, m_httpTimeout);

	Parser jsonParser;
	Dynamic::Var var = jsonParser.parse(response.getBody());
	Array::Ptr array = var.extract<Array::Ptr>();

	Object::Ptr first = array->getObject(0);
	if (!first->has("success"))
		logger().information("request to search new devices failed", __FILE__, __LINE__);
}

/**
 * Example of response's body with one bulb:
 * {
 *     "1": {
 *         "state": {
 *             "on": true,
 *             "bri": 51,
 *             "alert": "none",
 *             "reachable": true
 *         },
 *         "swupdate": {
 *             "state": "readytoinstall",
 *             "lastinstall": null
 *         },
 *         "type": "Dimmable light",
 *         "name": "Hue white lamp 1",
 *         "modelid": "LWB006",
 *         "manufacturername": "Philips",
 *         "uniqueid": "00:17:88:01:10:5c:34:5f-0b",
 *         "swversion": "5.38.1.15095"
 *     }
 * }
 */
list<pair<string, pair<uint32_t, PhilipsHueBridge::BulbID>>> PhilipsHueBridge::requestDeviceList()
{
	list<pair<string, pair<uint32_t, PhilipsHueBridge::BulbID>>> list;

	URI uri("/api/" + username() + "/lights");
	HTTPRequest request(HTTPRequest::HTTP_GET, uri.toString(), "HTTP/1.1");

	HTTPEntireResponse response = sendRequest(request, "", m_address, m_httpTimeout);

	Object::Ptr object = JsonUtil::parse(response.getBody());
	vector<string> lights;
	object->getNames(lights);

	for (auto light : lights) {
		Object::Ptr bulb = object->getObject(light);

		if (bulb->getObject("state")->getValue<bool>("reachable") == false)
			continue;

		string uniqueID = bulb->getValue<string>("uniqueid");
		string type = bulb->getValue<string>("type");

		list.push_back({type, {NumberParser::parseUnsigned(light), decodeBulbID(uniqueID)}});
	}

	return list;
}

/**
 * Example of request's body of setting brightness to 155:
 * {
 *     "bri":155
 * }
 *
 * Example of successful response's body:
 * [
 *     {
 *         "success": {
 *             "/lights/1/state/bri": 155
 *         }
 *     }
 * ]
 *
 * Example of failed response's body:
 * [
 *     {
 *         "error": {
 *             "type": 2,
 *             "address": "/lights/1/state",
 *             "description": "body contains invalid json"
 *         }
 *     }
 * ]
 */
bool PhilipsHueBridge::requestModifyState(const uint32_t ordinalNumber,
	const string& capability, const Dynamic::Var value)
{
	string stateMsg = requestDeviceState(ordinalNumber);
	Object::Ptr root = JsonUtil::parse(stateMsg);
	Object::Ptr state = root->getObject("state");

	if (state->getValue<bool>("reachable") == false)
		return false;

	ostringstream messageStream;
	Object object;
	object.set(capability, value);
	object.stringify(messageStream);

	URI uri("/api/" + username() + "/lights/" + to_string(ordinalNumber) + "/state");

	HTTPRequest request(HTTPRequest::HTTP_PUT, uri.toString(), "HTTP/1.1");
	request.setContentType("application/json");
	request.setContentLength(messageStream.str().length());

	HTTPEntireResponse response = sendRequest(request, messageStream.str(), m_address, m_httpTimeout);

	Parser jsonParser;
	Dynamic::Var var = jsonParser.parse(response.getBody());
	Array::Ptr array = var.extract<Array::Ptr>();

	for (uint32_t i = 0; i < array->size(); i++) {
		Object::Ptr first = array->getObject(i);
		if (!first->has("success"))
			return false;
	}

	return true;
}

/**
 * Example of response's body:
 * {
 *     "state": {
 *         "on": true,
 *         "bri": 51,
 *         "alert": "none",
 *         "reachable": true
 *     },
 *     "swupdate": {
 *         "state": "readytoinstall",
 *         "lastinstall": null
 *     },
 *     "type": "Dimmable light",
 *     "name": "Hue white lamp 1",
 *     "modelid": "LWB006",
 *     "manufacturername": "Philips",
 *     "uniqueid": "00:17:88:01:10:5c:34:5f-0b",
 *     "swversion": "5.38.1.15095"
 * }
 */
string PhilipsHueBridge::requestDeviceState(const uint32_t ordinalNumber)
{
	URI uri("/api/" + username() + "/lights/" + to_string(ordinalNumber));
	HTTPRequest request(HTTPRequest::HTTP_GET, uri.toString(), "HTTP/1.1");

	HTTPEntireResponse response = sendRequest(request, "", m_address, m_httpTimeout);
	return response.getBody();
}

SocketAddress PhilipsHueBridge::address() const
{
	return m_address;
}

void PhilipsHueBridge::setAddress(const SocketAddress& address)
{
	m_address = address;
}

MACAddress PhilipsHueBridge::macAddress() const
{
	return m_macAddr;
}

string PhilipsHueBridge::username() const
{
	if (!m_credential.isNull()) {
		CipherKey key = m_cryptoConfig->createKey(m_credential->params());
		Cipher *cipher = CipherFactory::defaultFactory().createCipher(key);

		return m_credential->username(cipher);
	}

	throw NotFoundException("username is not defined");
}

void PhilipsHueBridge::setCredentials(const SharedPtr<PasswordCredentials> credential,
	const SharedPtr<CryptoConfig> config)
{
	m_credential = credential;
	m_cryptoConfig = config;
}

uint32_t PhilipsHueBridge::countOfBulbs()
{
	return m_countOfBulbs;
}

FastMutex& PhilipsHueBridge::lock()
{
	return m_lock;
}

PhilipsHueBridgeInfo PhilipsHueBridge::info()
{
	URI uri("/api/beeeon/config");
	HTTPRequest request(HTTPRequest::HTTP_GET, uri.toString(), "HTTP/1.1");

	HTTPEntireResponse response = sendRequest(request, "", m_address, m_httpTimeout);
	return PhilipsHueBridgeInfo::buildBridgeInfo(response.getBody());
}

/**
 * Example of response's body:
 * {
 *     "name": "Philips hue",
 *     "datastoreversion": "63",
 *     "swversion": "1709131301",
 *     "apiversion": "1.21.0",
 *     "mac": "00:17:88:29:12:17",
 *     "bridgeid": "001788FFFE291217",
 *     "factorynew": false,
 *     "replacesbridgeid": null,
 *     "modelid": "BSB002",
 *     "starterkitid": ""
 * }
 */
void PhilipsHueBridge::requestDeviceInfo()
{
	URI uri("/api/beeeon/config");
	HTTPRequest request(HTTPRequest::HTTP_GET, uri.toString(), "HTTP/1.1");

	HTTPEntireResponse response = sendRequest(request, "", m_address, m_httpTimeout);

	Object::Ptr object = JsonUtil::parse(response.getBody());
	m_macAddr = MACAddress::parse(object->getValue<string>("mac"));
}

PhilipsHueBridge::BulbID PhilipsHueBridge::decodeBulbID(const string& strBulbID) const
{
	RegularExpression::MatchVec matches;
	RegularExpression re("([0-9a-fA-F]{2}:){7}[0-9a-fA-F]{2}");

	re.match(strBulbID, 0, matches);
	string bulbID = strBulbID.substr(matches[0].offset, matches[0].length);
	bulbID = replace(bulbID, ":", "");

	return NumberParser::parseHex64(bulbID);
}

void PhilipsHueBridge::incrementCountOfBulbs()
{
	ScopedLock<FastMutex> guard(m_lockCountOfBulbs);

	m_countOfBulbs++;
}

void PhilipsHueBridge::decrementCountOfBulbs()
{
	ScopedLock<FastMutex> guard(m_lockCountOfBulbs);

	if (m_countOfBulbs == 0)
		throw IllegalStateException("count of bulbs can not be negative");

	m_countOfBulbs--;
}

HTTPEntireResponse PhilipsHueBridge::sendRequest(HTTPRequest& request,
	const string& message, const SocketAddress& address, const Timespan& timeout)
{
	logger().debug("sending HTTP request to " + address.toString() +
		request.getURI(), __FILE__, __LINE__);

	return HTTPUtil::makeRequest(
		request, address.host().toString(), address.port(), message, timeout);
}
