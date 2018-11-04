#include "commands/DeviceUnpairResult.h"
#include "core/Answer.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

DeviceUnpairResult::DeviceUnpairResult(AutoPtr<Answer> answer):
	Result(answer)
{
}

void DeviceUnpairResult::setUnpaired(const set<DeviceID> &ids)
{
	m_unpaired = ids;
}

const set<DeviceID> &DeviceUnpairResult::unpaired() const
{
	return m_unpaired;
}
