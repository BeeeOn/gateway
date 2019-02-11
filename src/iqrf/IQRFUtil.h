#pragma once

#include <string>

#include <Poco/Logger.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "iqrf/DPARequest.h"
#include "iqrf/IQRFJsonResponse.h"
#include "iqrf/IQRFMqttConnector.h"

namespace BeeeOn {

class IQRFUtil {
public:
	IQRFUtil() = delete;
	IQRFUtil(const IQRFUtil &) = delete;
	IQRFUtil(const IQRFUtil &&) = delete;
	~IQRFUtil() = delete;

	/**
	 * @brief Send DPA request and wait for JSON response.
	 */
	static IQRFJsonResponse::Ptr makeRequest(
			IQRFMqttConnector::Ptr connector,
			DPARequest::Ptr,
			const Poco::Timespan &receiveTimeout
	);

private:
	static Poco::Logger &logger();
};

}
