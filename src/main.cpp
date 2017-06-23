#include "core/GatewayInfo.h"
#include "di/DIDaemon.h"

using namespace BeeeOn;

int main(int argc, char **argv)
{
	About about;

	about.requirePocoVersion = 0x01070000;
	about.recommendPocoVersion = 0x01070000;

	about.version = GatewayInfo::version();

	DIDaemon::up(argc, argv, about);
}
