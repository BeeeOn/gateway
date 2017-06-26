#include <Poco/Logger.h>

#include "net/VPTHTTPScanner.h"
#include "util/JsonUtil.h"

#define VPT_VENDOR "Thermona"
#define VPT_DEVICE "Regulator VPT LAN v1.0"
#define VPT_VERSION 2016021100

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::JSON;
using namespace Poco::Net;
using namespace std;

VPTHTTPScanner::VPTHTTPScanner(const string& path, UInt16 port, const IPAddress& minNetMask):
	AbstractHTTPScanner(path, port, minNetMask)
{
}

VPTHTTPScanner::~VPTHTTPScanner()
{
}

void VPTHTTPScanner::prepareRequest(Poco::Net::HTTPRequest& request)
{
	request.setMethod(HTTPRequest::HTTP_GET);
	request.setURI(path());
}

bool VPTHTTPScanner::isValidResponse(const string& response)
{
	logger().debug(response, __FILE__, __LINE__);

	string vendor;
	string device;
	int version;
	try {
		Object::Ptr object = JsonUtil::parse(response);

		vendor = object->getValue<string>("vendor");
		device = object->getValue<string>("device");
		version = object->getValue<int>("version");
	}
	catch (Exception& e) {
		if (logger().debug())
			logger().log(e, __FILE__, __LINE__);
		return false;
	}

	if (vendor != VPT_VENDOR) {
		logger().warning("unrecognized vendor " + vendor, __FILE__, __LINE__);
		return false;
	}

	if (device != VPT_DEVICE) {
		logger().warning("unrecognized device type " + device, __FILE__, __LINE__);
		return false;
	}

	if (version != VPT_VERSION) {
		logger().warning("unrecognized version " + to_string(version), __FILE__, __LINE__);
		return false;
	}

	logger().notice("found device" + vendor + " " + device, __FILE__, __LINE__);

	return true;
}
