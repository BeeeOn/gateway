#pragma once

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class ConradEvent;

/**
 * Provide an interface to enable delivering Conrad-related events to collectors.
 */
class ConradListener {
public:
	typedef Poco::SharedPtr<ConradListener> Ptr;

	virtual ~ConradListener();

	/**
	 * This method is called when some message is received or transmitted
	 */
	virtual void onConradMessage(const ConradEvent &info) = 0;
};

}
