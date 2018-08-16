#include <Poco/String.h>

#include "zwave/ZWavePocoLoggerAdapter.h"

using namespace BeeeOn;
using namespace Poco;
using namespace OpenZWave;
using namespace std;

ZWavePocoLoggerAdapter::ZWavePocoLoggerAdapter(Poco::Logger& logger):
	logger(logger)
{
}

void ZWavePocoLoggerAdapter::Write(LogLevel logLevel, uint8 const nodeId,
	char const* format, va_list args)
{
	Message msg;

	switch(logLevel) {
	case LogLevel_Fatal:
		msg.setPriority(Message::PRIO_FATAL);
		break;
	case LogLevel_Error:
		msg.setPriority(Message::PRIO_ERROR);
		break;
	case LogLevel_Warning:
		msg.setPriority(Message::PRIO_WARNING);
		break;
	case LogLevel_Alert:
		msg.setPriority(Message::PRIO_NOTICE);
		break;
	case LogLevel_Always:
	case LogLevel_Info:
		msg.setPriority(Message::PRIO_INFORMATION);
		break;
	case LogLevel_Detail:
	case LogLevel_Debug:
		msg.setPriority(Message::PRIO_DEBUG);
		break;
	case LogLevel_StreamDetail:
	case LogLevel_Internal:
		msg.setPriority(Message::PRIO_TRACE);
		break;
	default:
		msg.setPriority(Message::PRIO_DEBUG);
		break;
	}

	msg.setSource(logger.name());
	msg.set("node", to_string(nodeId));

	char lineBuf[1024] = {0};
	if (format != NULL && format[0] != '\0')
		vsnprintf(lineBuf, sizeof(lineBuf), format, args);

	string line(lineBuf);

	trimRightInPlace(line);

	if (line.empty())
		return;

	msg.setText(line);

	logger.log(msg);
}

LogLevel ZWavePocoLoggerAdapter::fromPocoLevel(Message::Priority prio)
{
	switch (prio) {
	case Message::PRIO_FATAL:
	case Message::PRIO_CRITICAL:
		return LogLevel_Fatal;
	case Message::PRIO_ERROR:
		return LogLevel_Error;
	case Message::PRIO_WARNING:
		return LogLevel_Warning;
	case Message::PRIO_NOTICE:
		return LogLevel_Alert;
	case Message::PRIO_INFORMATION:
		return LogLevel_Info;
	case Message::PRIO_DEBUG:
		return LogLevel_Detail;
	case Message::PRIO_TRACE:
		return LogLevel_StreamDetail;
	default:
		return LogLevel_Detail;
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
