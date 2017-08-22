#pragma once

#include <Poco/SharedPtr.h>

#include "zwave/ZWaveDriverEvent.h"
#include "zwave/ZWaveNodeEvent.h"

namespace BeeeOn {

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
};

}
