#include <Poco/Logger.h>

#include "core/CommandProgressHandler.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

void CommandProgressHandler::onStarted(
	Poco::TaskStartedNotification *notification)
{
	if (logger().debug()) {
		logger().debug(notification->task()->name()
			+ "has started");
	}

	notification->release();
}

void CommandProgressHandler::onFinished(
	Poco::TaskFinishedNotification *notification)
{
	if (logger().debug()) {
		logger().debug(notification->task()->name()
			+ "has finished");
	}

	notification->release();
}

void CommandProgressHandler::onCancel(
	Poco::TaskCancelledNotification *notification)
{
	if (logger().debug()) {
		logger().debug(notification->task()->name()
			+ "has canceled");
	}

	notification->release();
}

void CommandProgressHandler::onFailed(
	Poco::TaskFailedNotification *notification)
{
	logger().error(notification->task()->name() + " has failed",
		__FILE__, __LINE__);
	logger().log(notification->reason(), __FILE__, __LINE__);

	notification->release();
}
