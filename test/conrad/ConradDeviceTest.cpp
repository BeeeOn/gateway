#include <cppunit/extensions/HelperMacros.h>

#include <Poco/JSON/Object.h>

#include "conrad/PowerMeterSwitch.h"
#include "conrad/RadiatorThermostat.h"
#include "conrad/WirelessShutterContact.h"
#include "cppunit/BetterAssert.h"
#include "model/DeviceID.h"
#include "model/RefreshTime.h"
#include "model/SensorData.h"

using namespace Poco;
using namespace Poco::JSON;
using namespace std;

namespace BeeeOn {

class ConradDeviceTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ConradDeviceTest);
	CPPUNIT_TEST(testPowerMeterSwitchParseValidData);
	CPPUNIT_TEST(testRadiatorThermostatParseValidData);
	CPPUNIT_TEST(testWirelessShutterContactParseValidData);
	CPPUNIT_TEST_SUITE_END();
public:
	void testPowerMeterSwitchParseValidData();
	void testRadiatorThermostatParseValidData();
	void testWirelessShutterContactParseValidData();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ConradDeviceTest);

void ConradDeviceTest::testPowerMeterSwitchParseValidData()
{
	Object::Ptr channels = new Object();
	channels->set("Main", "CMDs_done");
	channels->set("Pwr", "32.6");
	channels->set("SenF", "50");
	channels->set("SenI", "120");
	channels->set("SenPwr", "5");
	channels->set("SenU", "240");
	channels->set("Sw", "off");

	Object::Ptr event = new Object();
	event->set("dev", "HM_38D649");
	event->set("event", "message");
	event->set("model", "HM-ES-PMSW1-PL");
	event->set("raw", "A1478845E38D6490000008001460000000000095A02");
	event->set("rssi", -35.5);
	event->set("serial", "MEQ0106579");
	event->set("type", "powerMeter");
	event->set("channels", channels);

	PowerMeterSwitch plug(DeviceID(), RefreshTime::DISABLED);
	SensorData data = plug.parseMessage(event);

	CPPUNIT_ASSERT_EQUAL(data[0].value(), 50);
	CPPUNIT_ASSERT_EQUAL(data[1].value(), 120);
	CPPUNIT_ASSERT_EQUAL(data[2].value(), 5);
	CPPUNIT_ASSERT_EQUAL(data[3].value(), 240);
	CPPUNIT_ASSERT_EQUAL(data[4].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data[5].value(), -35.5);
}

void ConradDeviceTest::testRadiatorThermostatParseValidData()
{
	Object::Ptr channels = new Object();
	channels->set("Main", "CMDs_done");
	channels->set("Clima", "T: 21.2 desired: 17.0 valve: 0");
	channels->set("ClimaTeam", "unpeered");
	channels->set("Climate", "unpeered");
	channels->set("Weather", "21.2");
	channels->set("WindowRec", "last:trigLast");
	channels->set("remote", "unpeered");

	Object::Ptr event = new Object();
	event->set("dev", "HM_36BA59");
	event->set("event", "message");
	event->set("model", "HM-CC-RT-DN");
	event->set("raw", "A0FE0861036BA590000000A88D40C0000");
	event->set("rssi", -41.5);
	event->set("serial", "MEQ0233325");
	event->set("type", "thermostat");
	event->set("channels", channels);

	RadiatorThermostat thermostat(DeviceID(), RefreshTime::DISABLED);
	SensorData data = thermostat.parseMessage(event);

	CPPUNIT_ASSERT_EQUAL(data[0].value(), 21.2);
	CPPUNIT_ASSERT_EQUAL(data[1].value(), 17);
	CPPUNIT_ASSERT_EQUAL(data[2].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data[3].value(), -41.5);
}

void ConradDeviceTest::testWirelessShutterContactParseValidData()
{
	Object::Ptr channels = new Object();
	channels->set("Main", "open");

	Object::Ptr event = new Object();
	event->set("dev", "HM_30B0BE");
	event->set("event", "message");
	event->set("model", "HM-SEC-SC-2");
	event->set("raw", "A0C44A64130B0BEF11034013FC8");
	event->set("rssi", -52);
	event->set("serial", "LEQ1101988");
	event->set("type", "threeStateSensor");
	event->set("channels", channels);

	WirelessShutterContact contact(DeviceID(), RefreshTime::DISABLED);
	SensorData data = contact.parseMessage(event);

	CPPUNIT_ASSERT_EQUAL(data[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data[1].value(), -52.0);
}

}
