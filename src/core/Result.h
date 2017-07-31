#ifndef BEEEON_RESULT_H
#define BEEEON_RESULT_H

#include <Poco/AutoPtr.h>
#include <Poco/Event.h>
#include <Poco/RefCountedObject.h>

#include "util/Castable.h"
#include "util/Enum.h"

namespace BeeeOn {

class Answer;

/*
 * Representation of the result that is created and set in
 * CommandHandler::handle(). Status of the Result is set to
 * PENDING after the result creation.
 *
 * The notification about the changed status is sent using notifyUpdated()
 * after the status is changed. It is automatically called in the
 * setStatus() method and it is processed by Answer. The setDirty(true)
 * is set after the notification setting. It means that the new result is
 * available in the Answer.
 *
 * The Answer and the Result share the common mutex. The operations that
 * change/get the state in the Answer and in the Result MUST be locked.
 * The methods in the Results are created for this purpose.
 */
class Result : public Poco::RefCountedObject, public Castable {
public:
	typedef Poco::AutoPtr<Result> Ptr;

	struct StatusEnum
	{
		enum Raw
		{
			PENDING,
			SUCCESS,
			FAILED,
		};

		static EnumHelper<Raw>::ValueMap &valueMap();
	};

	typedef Enum<StatusEnum> Status;

	Result(Poco::AutoPtr<Answer> answer);

	void setStatus(const Status status);

	/*
	 * It internally calls the notifyUpdated().
	 */
	void setStatusUnlocked(const Status status);

	Status status() const;
	Status statusUnlocked() const;

	/*
	 * Notifies the waiting threads that this Result (and its Answer) were
	 * changed. The call sets Answer::setDirtyUnlocked(true).
	 */
	void notifyUpdated();

	Poco::FastMutex &lock() const;

protected:
	/*
	 * The check if the operation is locked.
	 */
	void assureLocked() const;

	~Result();

private:
	Status m_status;
	Answer &m_answer;
};

}

#endif
