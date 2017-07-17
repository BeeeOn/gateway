#include "zwave/ZWavePocoLoggerAdapter.h"

const static int USB_DONGLE_NODE_ID = 0;

using namespace BeeeOn;
using namespace OpenZWave;
using namespace std;

ZWavePocoLoggerAdapter::ZWavePocoLoggerAdapter(Poco::Logger& logger):
	logger(logger)
{
}

string ZWavePocoLoggerAdapter::nodeIdString(uint8 const nodeId)
{
	if (nodeId == USB_DONGLE_NODE_ID)
		return "";

	return "NodeId: " + to_string(nodeId) + " ";
}

void ZWavePocoLoggerAdapter::Write(LogLevel logLevel, uint8 const nodeId,
	char const* format, va_list args)
{
	string logMessage;
	char lineBuf[1024] = {0};

	if (format != NULL && format[0] != '\0')
		vsnprintf(lineBuf, sizeof(lineBuf), format, args);

	logMessage.append(nodeIdString(nodeId));
	logMessage.append(lineBuf);

	if (logMessage.length() > 0)
		writeLogImpl(logLevel, logMessage);
}

void ZWavePocoLoggerAdapter::writeLogImpl(LogLevel level, string logMessage)
{
	switch(level) {
	case LogLevel_Fatal:
		logger.fatal(logMessage);
		break;
	case LogLevel_Error:
		logger.error(logMessage);
		break;
	case LogLevel_Warning:
		logger.warning(logMessage);
		break;
	case LogLevel_Alert:
		logger.notice(logMessage);
		break;
	case LogLevel_Always:
	case LogLevel_Info:
		logger.information(logMessage);
		break;
	case LogLevel_Detail:
	case LogLevel_Debug:
		logger.debug(logMessage);
		break;
	case LogLevel_StreamDetail:
	case LogLevel_Internal:
		logger.trace(logMessage);
	default:
		break;
	}
}

void ZWavePocoLoggerAdapter::QueueDump()
{
}

void ZWavePocoLoggerAdapter::QueueClear()
{
}

void ZWavePocoLoggerAdapter::SetLoggingState(LogLevel, LogLevel, LogLevel)
{
}

void ZWavePocoLoggerAdapter::SetLogFileName(const string &)
{
}
