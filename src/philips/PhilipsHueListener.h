#pragma once

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class PhilipsHueBridgeInfo;
class PhilipsHueBulbInfo;

/**
 * Interface for reporting statistics from PhilipsHueDeviceManager.
 */
class PhilipsHueListener {
public:
	typedef Poco::SharedPtr<PhilipsHueListener> Ptr;

	virtual ~PhilipsHueListener();

	/**
	 * This method is called when statistics of Philips Hue Bulb
	 * is sent from device manager.
	 */
	virtual void onBulbStats(const PhilipsHueBulbInfo &info) = 0;

	/**
	 * This method is called when statistics of Philips Hue Bridge
	 * is sent from device manager.
	 */
	virtual void onBridgeStats(const PhilipsHueBridgeInfo &info) = 0;
};

}
