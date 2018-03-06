#include "zwave/ZWavePocoLoggerAdapter.h"

const static int USB_DONGLE_NODE_ID = 0;

using namespace BeeeOn;
using namespace Poco;
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

	char lineBuf[1024] = {0};
	if (format != NULL && format[0] != '\0')
		vsnprintf(lineBuf, sizeof(lineBuf), format, args);

	string line;
	line.append(nodeIdString(nodeId));
	line.append(lineBuf);

	trimRightInPlace(line);

	if (line.empty())
		return;

	msg.setText(line);

	logger.log(msg);
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
