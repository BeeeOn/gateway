#ifndef BEEEON_COMMAND_PROGRESS_HANDLER_H
#define BEEEON_COMMAND_PROGRESS_HANDLER_H

#include <set>

#include <Poco/BasicEvent.h>
#include <Poco/Mutex.h>
#include <Poco/Task.h>
#include <Poco/TaskNotification.h>

#include "util/Loggable.h"

namespace BeeeOn {

class Answer;

/*
 * The class checks the status of individual tasks for a given Answer.
 */
class CommandProgressHandler : public Loggable {
public:
	friend Answer;
	/*
	 * Event after started handle of Command.
	 */
	void onStarted(Poco::TaskStartedNotification *notification);

	/*
	 * Event after finished handle of Command.
	 */
	void onFinished(Poco::TaskFinishedNotification *notification);

	/*
	 * It informs about the canceled Command.
	 */
	void onCancel(Poco::TaskCancelledNotification *notification);

	/*
	 * Event after  failed handle of Command. It informs
	 * the exception that occurred in handle in CommandHandler.
	 */
	void onFailed(Poco::TaskFailedNotification *notification);
};

}


#endif
