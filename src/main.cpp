#include "di/DIDaemon.h"

using namespace BeeeOn;

int main(int argc, char **argv)
{
	About about;

	about.requirePocoVersion = 0x01070000;
	about.recommendPocoVersion = 0x01070000;

#ifdef GIT_ID
	about.version = GIT_ID;
#else
	about.version = "2017.03-rc1";
#endif

	DIDaemon::up(argc, argv, about);
}
