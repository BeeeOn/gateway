#include "bluetooth/HciUtil.h"

using namespace std;
using namespace BeeeOn;

string HciUtil::hotplugMatch(const HotplugEvent &e)
{
	if (!e.properties()->has("bluetooth.BEEEON_DONGLE"))
		return "";

	const auto &dongle = e.properties()->getString("bluetooth.BEEEON_DONGLE");

	if (dongle != "bluetooth")
		return "";

	return e.name();
}
