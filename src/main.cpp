#include "core/GatewayInfo.h"
#include "di/DIDaemon.h"
#include "util/PosixSignal.h"

using namespace BeeeOn;

int main(int argc, char **argv)
{
	About about;

	about.requirePocoVersion = 0x01070000;
	about.recommendPocoVersion = 0x01070000;

	about.version = GatewayInfo::version();
	PosixSignal::handle("SIGUSR1", [](int) {});

	DIDaemon::up(argc, argv, about);
}
