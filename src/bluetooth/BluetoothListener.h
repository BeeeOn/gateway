#ifndef BEEEON_BLUETOOTH_LISTENER_H
#define BEEEON_BLUETOOTH_LISTENER_H

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class HciInfo;

class BluetoothListener {
public:
	typedef Poco::SharedPtr<BluetoothListener> Ptr;

	virtual ~BluetoothListener();

	virtual void onHciStats(const HciInfo &info) = 0;
};

}

#endif
