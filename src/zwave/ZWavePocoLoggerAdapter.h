#pragma once

#include <Poco/Logger.h>
#include <Poco/Message.h>

#include <openzwave/platform/unix/LogImpl.h>

namespace BeeeOn {

class ZWavePocoLoggerAdapter : public OpenZWave::i_LogImpl {
public:
	ZWavePocoLoggerAdapter(Poco::Logger& logger);

	/**
	 * Write formatted data from variable argument list to string.
	 * @param logLevel log level used in openzwave
	 * @param nodeId idetifier in the z-wave network
	 * @param format message for logger
	 * @param args
	 */
	void Write(OpenZWave::LogLevel logLevel, uint8 const nodeId,
		char const* format, va_list args) override;

	void QueueDump() override;
	void QueueClear() override;

	void SetLoggingState(OpenZWave::LogLevel saveLevel,
		OpenZWave::LogLevel queueLevel,
		OpenZWave::LogLevel dumpTrigger) override;

	void SetLogFileName(const std::string& filename) override;

private:
	/**
	 * Converts nodeId identification to string with suffix.
	 * If the nodeId is 0, it is driver nodeId and it is skipped.
	 * For example:
	 *   "NodeId: 0 "
	 * @param nodeId identifier in the z-wave network
	 * @return string with node id
	 */
	std::string nodeIdString(uint8 const nodeId);

private:
	Poco::Logger& logger;
};

}
