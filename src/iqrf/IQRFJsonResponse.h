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
			STATUS_NO_ERROR,
			ERROR_FAIL,
			ERROR_PCMD,
			ERROR_PNUM,
			ERROR_ADDR,
			ERROR_DATA_LEN,
			ERROR_DATA,
			ERROR_HWPID,
			ERROR_NADR,
			ERROR_IFACE_CUSTOM_HANDLER,
			ERROR_MISSING_CUSTOM_DPA_HANDLER,
			ERROR_TIMEOUT,
			STATUS_CONFIRMATION =  0xFF
		};

		static EnumHelper<Raw>::ValueMap &valueMap();
	};

	typedef Enum<DpaErrorEnum> DpaError;

	/**
	 * @brief The structure corresponds to the composition of the field contained
	 * within the message data->raw
	 * https://apidocs.iqrf.org/iqrf-gateway-daemon/json/#iqrf/iqrfRaw-response-1-0-0.json
	 */
	struct RawData {
		std::string request;
		std::string requestTs;
		std::string confirmation;
		std::string confirmationTs;
		std::string response;
		std::string responseTs;
	};

	IQRFJsonResponse();

	/**
	 * @param response contains hex values separated by dot.
	 */
	void setResponse(const std::string &response);
	std::string response() const;

	/**
	 * @param raw Content of raw data array inside JSON data object
	 */
	void setRawData(const RawData &raw);

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

	/**
	 * @param stat_string GW daemon API status in string form
	 * @param stat_num GW daemon API status
	 */
	void setStatus(const std::string& statString, int statNum);

	/**
	 * @param identification IQRF GW daemon instance identification
	 */
	void setGWIdentification(const std::string& identification);

	/**
	 * @param request contains hex values separated by dot.
	 */
	void setRequest(const std::string &request) override;
	std::string request() const  override;

private:
	RawData m_rawData;

	DpaError m_errorCode;

	std::string m_insId;
	std::string m_statusStr;
	int m_status = 0;
};

}
