#pragma once

#include "bluetooth/HciListener.h"
#include "conrad/ConradListener.h"
#include "core/CommandDispatcherListener.h"
#include "core/DistributorListener.h"
#include "iqrf/IQRFListener.h"
#include "philips/PhilipsHueListener.h"
#include "zwave/ZWaveListener.h"

namespace BeeeOn {

/**
 * @brief The class represents a collector of events occuring inside the
 * Gateway. It implements all available listeners that can potentially
 * provide some interesting events about the Gateway and the connected
 * sensors.
 */
class AbstractCollector :
	public HciListener,
	public DistributorListener,
	public PhilipsHueListener,
	public ZWaveListener,
	public CommandDispatcherListener,
	public IQRFListener,
	public ConradListener {
public:
	virtual ~AbstractCollector();

	/**
	 * Empty implementation to be overrided if needed.
	 */
	void onExport(const SensorData &data) override;

	/**
	 * Empty implementation to be overrided if needed.
	 */
	void onDriverStats(const ZWaveDriverEvent &event) override;

	/**
	 * Empty implementation to be overrided if needed.
	 */
	void onNodeStats(const ZWaveNodeEvent &event) override;

	/**
	 * Empty implementation to be overrided if needed.
	 */
	void onNotification(const OZWNotificationEvent &event) override;

	/**
	 * Empty implementation to be overrided if needed.
	 */
	void onHciStats(const HciInfo &info) override;

	/**
	 * Empty implementation to be overrided if needed.
	 */
	void onBulbStats(const PhilipsHueBulbInfo &info) override;

	/**
	 * Empty implementation to be overrided if needed.
	 */
	void onBridgeStats(const PhilipsHueBridgeInfo &info) override;

	/**
	 * Empty implementation to be overrided if needed.
	 */
	void onDispatch(const Command::Ptr cmd) override;

	/**
	 * Empty implementation to be overrided if needed.
	 */
	void onReceiveDPA(const IQRFEvent &info) override;

	/**
	 * Empty implementation to be overrided if needed.
	 */
	void onConradMessage(const ConradEvent &info) override;
};

}
