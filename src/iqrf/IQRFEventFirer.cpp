#include <Poco/Logger.h>

#include "iqrf/IQRFEvent.h"
#include "iqrf/IQRFEventFirer.h"
#include "iqrf/IQRFListener.h"
#include "util/EventSource.h"

using namespace BeeeOn;
using namespace Poco;

IQRFEventFirer::IQRFEventFirer()
{
}

void IQRFEventFirer::setAsyncExecutor(AsyncExecutor::Ptr executor)
{
	m_eventSource.setAsyncExecutor(executor);
}

void IQRFEventFirer::addListener(IQRFListener::Ptr listener)
{
	m_eventSource.addListener(listener);
}

void IQRFEventFirer::fireDPAStatistics(const DPAResponse::Ptr DPA)
{
	try {
		IQRFEvent event(DPA);
		m_eventSource.fireEvent(event, &IQRFListener::onReceiveDPA);
	}
	catch (Exception& e) {
		logger().warning("failed to obtain information from DPA response");
		logger().log(e, __FILE__, __LINE__);
	}
}

void IQRFEventFirer::fireDPAStatistics(const DPARequest::Ptr DPA)
{
	try {
		IQRFEvent event(DPA);
		m_eventSource.fireEvent(event, &IQRFListener::onReceiveDPA);
	}
	catch (Exception& e) {
		logger().warning("failed to obtain information from DPA request");
		logger().log(e, __FILE__, __LINE__);
	}
}
