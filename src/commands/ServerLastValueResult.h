#ifndef BEEEON_SERVER_LAST_VALUE_RESULT_H
#define BEEEON_SERVER_LAST_VALUE_RESULT_H

#include "core/Answer.h"
#include "core/Result.h"
#include "model/DeviceID.h"
#include "model/ModuleID.h"

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

	void setDeviceID(const DeviceID &deviceID);
	void setDeviceIDUnlocked(const DeviceID &deviceID);

	DeviceID deviceID() const;
	DeviceID deviceIDUnlocked() const;

	void setModuleID(const ModuleID &moduleID);
	void setModuleIDUnlocked(const ModuleID &moduleID);

	ModuleID moduleID() const;
	ModuleID moduleIDUnlocked() const;

protected:
	~ServerLastValueResult();

private:
	double m_value;
	DeviceID m_deviceID;
	ModuleID m_moduleID;
};

}

#endif
