#pragma once

#include "iqrf/IQRFJsonMessage.h"

namespace BeeeOn {

class IQRFJsonRequest : public IQRFJsonMessage {
public:
	typedef Poco::SharedPtr<IQRFJsonRequest> Ptr;

	IQRFJsonRequest();

	/**
	 * @param request contains hex values separated by dot.
	 */
	virtual void setRequest(const std::string &request);
	virtual std::string request() const;

	/**
	 * @return Converts all data to one JSON string.
	 *
	 * @note [OPTIONAL] It is necessary to keep the correct order
	 * of elements inside JSON. otherwise, some DPA messages
	 * such as bindings do not work
	 * @note Based on documentation of APIv2,
	 * https://apidocs.iqrf.org/iqrf-gateway-daemon/json/#iqrf/iqrfRaw-request-1-0-0.json
	 */
	std::string toString() override;

private:
	std::string m_request;
};

}
