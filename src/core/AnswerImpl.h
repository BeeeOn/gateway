#ifndef BEEEON_ANSWER_IMPL_H
#define BEEEON_ANSWER_IMPL_H

#include <Poco/SharedPtr.h>

namespace BeeeOn {

/**
 * A common ancestor of all implementation-specific logic inside Answer.
 */
class AnswerImpl {
public:
	typedef Poco::SharedPtr<AnswerImpl> Ptr;

	virtual ~AnswerImpl()
	{
	}
};

}

#endif
