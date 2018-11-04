#pragma once

#include "iqrf/IQRFJsonRequest.h"
#include "util/Enum.h"

namespace BeeeOn {

class IQRFJsonResponse : public IQRFJsonRequest {
public:
	typedef Poco::SharedPtr<IQRFJsonResponse> Ptr;

	struct DpaErrorEnum
	{
		enum Raw {
			ERROR_FAIL,
			ERROR_IFACE,
			ERROR_NADR,
			ERROR_PNUM,
			ERROR_TIMEOUT,
			STATUS_NO_ERROR,
		};

		static EnumHelper<Raw>::ValueMap &valueMap();
	};

	typedef Enum<DpaErrorEnum> DpaError;

	IQRFJsonResponse();

	/**
	 * @param response contains hex values separated by dot.
	 */
	void setResponse(const std::string &response);
	std::string response() const;

	/**
	 * @return Converts all data to one JSON string.
	 */
	std::string toString() override;

	/**
	 * DPA error from IQRF daemon. Every error code is represented using
	 * string that describes DPA error.
	 */
	DpaError errorCode() const;
	void setErrorCode(const DpaError &errCode);

private:
	std::string m_response;
	DpaError m_errorCode;
};

}
