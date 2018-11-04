#pragma once

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class ZWaveDriverEvent;
class ZWaveNodeEvent;
class OZWNotificationEvent;

/**
 * Interface for reporting statistics from ZWaveDeviceManager.
 */
class ZWaveListener {
public:
	typedef Poco::SharedPtr<ZWaveListener> Ptr;

	ZWaveListener();
	virtual ~ZWaveListener();

	/**
	 * This method is called when statistics of ZWave USB driver
	 * is sent from ZWaveDeviceManger.
	 */
	virtual void onDriverStats(const ZWaveDriverEvent &nodeEvent) = 0;

	/**
	 * This method is called when statistics of ZWave device (node)
	 * is sent from ZWaveDeviceManger.
	 */
	virtual void onNodeStats(const ZWaveNodeEvent &nodeEvent) = 0;

	/**
	 * This method is called for each low-level Z-Wave notification.
	 */
	virtual void onNotification(const OZWNotificationEvent &e) = 0;
};

}
