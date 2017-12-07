#include <string>

#include <Poco/Logger.h>

#include "bluetooth/HciInfo.h"
#include "core/LoggingCollector.h"
#include "di/Injectable.h"
#include "model/SensorData.h"
#include "util/Occasionally.h"
#include "zwave/ZWaveDriverEvent.h"
#include "zwave/ZWaveNodeEvent.h"

BEEEON_OBJECT_BEGIN(BeeeOn, LoggingCollector)
BEEEON_OBJECT_CASTABLE(DistributorListener)
BEEEON_OBJECT_CASTABLE(ZWaveListener)
BEEEON_OBJECT_CASTABLE(BluetoothListener)
BEEEON_OBJECT_CASTABLE(CommandDispatcherListener)
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


void LoggingCollector::onDispatch(const Command::Ptr cmd)
{
	logger().information("dispatch " + cmd->toString());
}
