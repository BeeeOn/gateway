#pragma once

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class HciInfo;

class HciListener {
public:
	typedef Poco::SharedPtr<HciListener> Ptr;

	virtual ~HciListener();

	virtual void onHciStats(const HciInfo &info) = 0;
};

}
