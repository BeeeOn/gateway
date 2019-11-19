#pragma once

#include <Poco/SharedPtr.h>

#include "iqrf/DPARequest.h"
#include "iqrf/DPAResponse.h"
#include "iqrf/IQRFListener.h"
#include "util/EventSource.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * The intetion of this class is to export IQRFEvents to all IQRFListener.
 * It encapsulates the logic of firing the IQRFEvents because the reference
 * of this class is passed to all IQRFDevices to send statistic about their
 * communication.
 */
class IQRFEventFirer: protected Loggable {
public:
	typedef Poco::SharedPtr<IQRFEventFirer> Ptr;

	IQRFEventFirer();

	void fireDPAStatistics(const DPAResponse::Ptr DPA);
	void fireDPAStatistics(const DPARequest::Ptr DPA);

	void setAsyncExecutor(AsyncExecutor::Ptr executor);
	void addListener(IQRFListener::Ptr listener);

private:
	EventSource<IQRFListener> m_eventSource;
};

}
