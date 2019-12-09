#pragma once

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class IQRFEvent;

/**
 * Provide an interface to enable delivering IQRF-related events to collectors.
 */
class IQRFListener {
public:
	typedef Poco::SharedPtr<IQRFListener> Ptr;

	virtual ~IQRFListener();

	/**
	 * This method is called when DPA message is received from IQRF
	 * gateway daemon.
	 */
	virtual void onReceiveDPA(const IQRFEvent &info) = 0;
};

}
