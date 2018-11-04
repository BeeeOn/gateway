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
	void setRequest(const std::string &request);
	std::string request() const;

	/**
	 * @return Converts all data to one JSON string.
	 */
	std::string toString() override;

private:
	std::string m_request;
};

}
