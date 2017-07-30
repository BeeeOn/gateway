#ifndef BEEEON_UDEV_LISTENER_H
#define BEEEON_UDEV_LISTENER_H

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class UDevEvent;

class UDevListener {
public:
	typedef Poco::SharedPtr<UDevListener> Ptr;

	virtual ~UDevListener();

	virtual void onAdd(const UDevEvent &event);
	virtual void onRemove(const UDevEvent &event);
	virtual void onChange(const UDevEvent &event);
	virtual void onMove(const UDevEvent &event);
};

}

#endif
