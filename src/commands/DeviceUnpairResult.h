#pragma once

#include <set>

#include "core/Result.h"
#include "model/DeviceID.h"

namespace BeeeOn {

/**
 * @brief DeviceUnpairResult holds set of devices that have
 * been unpaired. If the set is empty, no device have been
 * unpaired but the operation was successful.
 *
 * The result makes possible to change the device ID asked
 * to be unpaired to another one or to multple ones.
 */
class DeviceUnpairResult : public Result {
public:
	typedef Poco::AutoPtr<DeviceUnpairResult> Ptr;

	DeviceUnpairResult(Poco::AutoPtr<Answer> answer);

	void setUnpaired(const std::set<DeviceID> &ids);
	const std::set<DeviceID> &unpaired() const;

private:
	std::set<DeviceID> m_unpaired;
};

}
