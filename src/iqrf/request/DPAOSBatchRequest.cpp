#include <Poco/NumberFormatter.h>

#include "iqrf/request/DPAOSBatchRequest.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const uint8_t BATCH_CMD = 0x05;
static const size_t DPA_NADR_SIZE = 2;
static const size_t DPA_SUBREQ_SIZE_BYTE = 1;
static const size_t DPA_NADR_WITH_SEPARATOR_CHAR_SIZE = 6;

DPABatchRequest::DPABatchRequest(uint8_t node):
	DPARequest(
		node,
		DPA_OS_PNUM,
		BATCH_CMD
	)
{
}

void DPABatchRequest::append(DPARequest::Ptr request)
{
	m_requests.emplace_back(request);
}

string DPABatchRequest::toDPAString() const
{
	string repr;

	repr += DPARequest::toDPAString();

	for (auto req : m_requests) {
		repr += ".";

		/*
		 * Total size of one request, that will be written to batch request:
		 *  - size of request without network address, first 2 B (-2 B)
		 *  - one byte with size of request (+1 B)
		 */
		const size_t sizeByte = req->size() - DPA_NADR_SIZE + DPA_SUBREQ_SIZE_BYTE;
		repr += NumberFormatter::formatHex(sizeByte, 2);
		repr += ".";

		// skip NADR (first 6 B, 4 B for number and 2 for separator)
		repr += req->toDPAString().substr(DPA_NADR_WITH_SEPARATOR_CHAR_SIZE, -1);
	}

	return repr;
}


