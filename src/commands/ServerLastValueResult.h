#pragma once

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
	double value() const;

	void setDeviceID(const DeviceID &deviceID);
	DeviceID deviceID() const;

	void setModuleID(const ModuleID &moduleID);
	ModuleID moduleID() const;

protected:
	~ServerLastValueResult();

private:
	double m_value;
	DeviceID m_deviceID;
	ModuleID m_moduleID;
};

}
