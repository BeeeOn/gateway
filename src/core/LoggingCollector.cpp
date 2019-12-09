#include <string>

#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>

#include "bluetooth/HciInfo.h"
#include "core/LoggingCollector.h"
#include "di/Injectable.h"
#include "iqrf/IQRFEvent.h"
#include "model/SensorData.h"
#include "philips/PhilipsHueBulbInfo.h"
#include "philips/PhilipsHueBridgeInfo.h"
#include "util/Occasionally.h"

#ifdef HAVE_ZWAVE
#include "zwave/ZWaveDriverEvent.h"
#include "zwave/ZWaveNodeEvent.h"
#endif

#ifdef HAVE_OPENZWAVE
#include "zwave/OZWNotificationEvent.h"
#endif

#ifdef HAVE_IQRF
#include "iqrf/IQRFEvent.h"
#endif

BEEEON_OBJECT_BEGIN(BeeeOn, LoggingCollector)
BEEEON_OBJECT_CASTABLE(DistributorListener)
BEEEON_OBJECT_CASTABLE(ZWaveListener)
BEEEON_OBJECT_CASTABLE(HciListener)
BEEEON_OBJECT_CASTABLE(PhilipsHueListener)
BEEEON_OBJECT_CASTABLE(CommandDispatcherListener)
BEEEON_OBJECT_CASTABLE(IQRFListener)
BEEEON_OBJECT_END(BeeeOn, LoggingCollector)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

// frequency of reporting of sensor data
#define SENSOR_DATA_FREQ 7

LoggingCollector::LoggingCollector():
	m_seenData(0)
{
}

LoggingCollector::~LoggingCollector()
{
}

void LoggingCollector::onExport(const SensorData &)
{
	static Occasionally occasionally(SENSOR_DATA_FREQ);

	++m_seenData;

	occasionally.execute([&]() {
		logger().information("seen " + to_string(m_seenData) + " data");
	});
}

#ifdef HAVE_ZWAVE
void LoggingCollector::onDriverStats(const ZWaveDriverEvent &e)
{
	logger().information("Z-Wave Driver: "
			+ to_string(e.readCount())
			+ "/"
			+ to_string(e.writeCount())
			+ "/"
			+ to_string(e.CANCount())
			+ "/"
			+ to_string(e.NAKCount())
			+ "/"
			+ to_string(e.ACKCount())
			+ "/"
			+ to_string(e.dropped())
			+ "/"
			+ to_string(e.badChecksum()));
}

void LoggingCollector::onNodeStats(const ZWaveNodeEvent &e)
{
	logger().information("Z-Wave Node: "
			+ to_string(e.nodeID())
			+ "/"
			+ to_string(e.sentCount())
			+ "/"
			+ to_string(e.sentFailed())
			+ "/"
			+ to_string(e.retries())
			+ "/"
			+ to_string(e.receivedCount())
			+ "/"
			+ to_string(e.receiveDuplications())
			+ "/"
			+ to_string(e.receiveUnsolicited())
			+ "/"
			+ to_string(e.quality()));
}
#else
void LoggingCollector::onDriverStats(const ZWaveDriverEvent &)
{
}

void LoggingCollector::onNodeStats(const ZWaveNodeEvent &)
{
}
#endif

#ifdef HAVE_OPENZWAVE
void LoggingCollector::onNotification(const OZWNotificationEvent &e)
{
	const string event = e.event().isNull() ?
		"(null)" : NumberFormatter::formatHex(e.event().value(), 2, true);

	logger().debug("OpenZWave Notification: "
			+ to_string(e.type())
			+ ", {"
			+ NumberFormatter::formatHex(e.homeID(), 8, true)
			+ ", "
			+ NumberFormatter::formatHex(e.nodeID(), 2, true)
			+ ", "
			+ to_string(e.valueID().GetGenre())
			+ ", "
			+ NumberFormatter::formatHex(e.valueID().GetCommandClassId(), 2, true)
			+ ", "
			+ NumberFormatter::formatHex(e.valueID().GetInstance(), 2, true)
			+ ", "
			+ NumberFormatter::formatHex(e.valueID().GetIndex(), 2, true)
			+ ", "
			+ to_string(e.valueID().GetType())
			+ "}, "
			+ NumberFormatter::formatHex(e.byte(), 2, true)
			+ ", "
			+ event);
}
#else
void LoggingCollector::onNotification(const OZWNotificationEvent &)
{
}
#endif

#ifdef HAVE_HCI
void LoggingCollector::onHciStats(const HciInfo &info)
{
	logger().information("HCI: "
			+ info.name()
			+ " "
			+ to_string(info.rxBytes())
			+ "/"
			+ to_string(info.rxErrors())
			+ "/"
			+ to_string(info.rxEvents())
			+ " "
			+ to_string(info.txBytes())
			+ "/"
			+ to_string(info.txErrors())
			+ "/"
			+ to_string(info.txCmds())
			+ " "
			+ to_string(info.aclPackets())
			+ "/"
			+ to_string(info.scoPackets()));
}
#else
void LoggingCollector::onHciStats(const HciInfo &)
{
}
#endif

#ifdef HAVE_PHILIPS_HUE
void LoggingCollector::onBulbStats(const PhilipsHueBulbInfo &info)
{
	string modules;
	for (auto module : info.modules())
		modules += module.first + ":" + module.second + ",";
	if (!modules.empty())
		modules.pop_back();

	logger().information("Philips Hue dimmable bulb: "
			+ modules
			+ "/"
			+ to_string(info.reachable())
			+ "/"
			+ info.type()
			+ "/"
			+ info.name()
			+ "/"
			+ info.modelId()
			+ "/"
			+ info.manufacturerName()
			+ "/"
			+ info.uniqueId()
			+ "/"
			+ info.swVersion());
}

void LoggingCollector::onBridgeStats(const PhilipsHueBridgeInfo &info)
{
	logger().information("Philips Hue bridge: "
			+ info.name()
			+ "/"
			+ info.dataStoreVersion()
			+ "/"
			+ info.swVersion()
			+ "/"
			+ info.apiVersion()
			+ "/"
			+ info.mac().toString()
			+ "/"
			+ info.bridgeId()
			+ "/"
			+ to_string(info.factoryNew())
			+ "/"
			+ info.modelId());
}
#else
void LoggingCollector::onBulbStats(const PhilipsHueBulbInfo &)
{
}

void LoggingCollector::onBridgeStats(const PhilipsHueBridgeInfo &)
{
}
#endif


#ifdef HAVE_IQRF
void LoggingCollector::onReceiveDPA(const IQRFEvent &info)
{
	logger().information("IQRF event: from address: "
			+ to_string(info.networkAddress())
			+ " payload size: "
			+ to_string(info.payload().size()));
}
#else
void LoggingCollector::onReceiveDPA(const IQRFEvent &info)
{
}
#endif

void LoggingCollector::onDispatch(const Command::Ptr cmd)
{
	logger().information("dispatch " + cmd->toString());
}
