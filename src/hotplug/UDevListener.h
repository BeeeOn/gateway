#ifndef BEEEON_UDEV_LISTENER_H
#define BEEEON_UDEV_LISTENER_H

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class HotplugEvent;

class UDevListener {
public:
	typedef Poco::SharedPtr<UDevListener> Ptr;

	virtual ~UDevListener();

	virtual void onAdd(const HotplugEvent &event);
	virtual void onRemove(const HotplugEvent &event);
	virtual void onChange(const HotplugEvent &event);
	virtual void onMove(const HotplugEvent &event);
};

}

#endif
