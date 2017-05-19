#ifndef BEEEON_SERVER_LAST_VALUE_RESULT_H
#define BEEEON_SERVER_LAST_VALUE_RESULT_H

#include "core/Answer.h"
#include "core/Result.h"

namespace BeeeOn {

/*
 * The result for ServerLastValueCommand that includes
 * the last value saved in database.
 */
class ServerLastValueResult : public Result {
public:
	typedef Poco::AutoPtr<ServerLastValueResult> Ptr;

	ServerLastValueResult(const Answer::Ptr answer);

	void setValue(double value);
	void setValueUnlocked(double value);

	double value() const;
	double valueUnlocked() const;

protected:
	~ServerLastValueResult();

private:
	double m_value;
};

}

#endif
