#pragma once

#include <Poco/Logger.h>
#include <Poco/Message.h>

#include <openzwave/platform/unix/LogImpl.h>

namespace BeeeOn {

/**
 * @brief ZWavePocoLoggerAdapter adapts the the logging infrastructure
 * of the OpenZWave library to the Poco::Logger. It converts the
 * OpenZWave logging levels as follows:
 *
 * - Fatal    - PRIO_FATAL
 * - Error    - PRIO_ERROR
 * - Warning  - PRIO_WARNING
 * - Alert    - PRIO_NOTICE
 * - Always   - PRIO_INFORMATION
 * - Info     - PRIO_INFORMATION
 * - Detail   - PRIO_DEBUG
 * - Debug    - PRIO_DEBUG
 * - StreamDetail - PRIO_TRACE
 * - Internal - PRIO_TRACE
 *
 * Each created Poco::Message would also contain named parameter "node"
 * holding the node ID. This can be used for better formatting of log
 * messages.
 */
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

	/**
	 * @brief The implementation does nothing.
	 */
	void QueueDump() override;

	/**
	 * @brief The implementation does nothing.
	 */
	void QueueClear() override;

	/**
	 * @brief The implementation does nothing.
	 */
	void SetLoggingState(OpenZWave::LogLevel saveLevel,
		OpenZWave::LogLevel queueLevel,
		OpenZWave::LogLevel dumpTrigger) override;

	/**
	 * @brief The implementation does nothing.
	 */
	void SetLogFileName(const std::string& filename) override;

	static OpenZWave::LogLevel fromPocoLevel(int prio)
	{
		return fromPocoLevel(
			static_cast<Poco::Message::Priority>(prio));
	}

	static OpenZWave::LogLevel fromPocoLevel(Poco::Message::Priority prio);

private:
	Poco::Logger& logger;
};

}
