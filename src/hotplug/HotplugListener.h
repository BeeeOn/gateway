#pragma once

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class HotplugEvent;

class HotplugListener {
public:
	typedef Poco::SharedPtr<HotplugListener> Ptr;

	virtual ~HotplugListener();

	virtual void onAdd(const HotplugEvent &event);
	virtual void onRemove(const HotplugEvent &event);
	virtual void onChange(const HotplugEvent &event);
	virtual void onMove(const HotplugEvent &event);
};

}
