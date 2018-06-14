#pragma once

#include "iqrf/DPARequest.h"

namespace BeeeOn {

/**
 * @brief One batch request can contain several
 * simple requests.
 */
class DPABatchRequest : public DPARequest {
public:
	typedef Poco::SharedPtr<DPABatchRequest> Ptr;

	/**
	 * @brief Insert many DPARequest to one batch request.
	 */
	void append(DPARequest::Ptr request);

	/**
	 * @brief Converts the header items and appended requests (without NADR)
	 * to one string that is divided by dots. Total size of one request:
	 * size of request - size of NADR + one byte for size of subrequest.
	 */
	std::string toDPAString() const override;

	DPABatchRequest(uint8_t node);

private:
	std::vector<DPARequest::Ptr> m_requests;
};

}
