#include <cppunit/extensions/HelperMacros.h>
#include <Poco/Crypto/Cipher.h>
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/CipherKey.h>
#include <Poco/Environment.h>
#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/Util/MapConfiguration.h>

#include "cppunit/BetterAssert.h"

#include "credentials/CredentialsStorage.h"
#include "credentials/PinCredentials.h"
#include "credentials/PasswordCredentials.h"

using namespace std;
using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Util;
using namespace Poco::Crypto;

namespace BeeeOn {

class CredentialsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(CredentialsTest);
	CPPUNIT_TEST(testCrypt);
	CPPUNIT_TEST(testSave);
	CPPUNIT_TEST(testLoad);
	CPPUNIT_TEST_SUITE_END();

public:
	CredentialsTest();

	void setUp() override;

	void testCrypt();
	void testSave();
	void testLoad();

private:
	CipherFactory &m_factory;
	string m_cipherName;
	CryptoParams m_params;
};

CPPUNIT_TEST_SUITE_REGISTRATION(CredentialsTest);

CredentialsTest::CredentialsTest():
	m_factory(CipherFactory::defaultFactory())
{
}

void CredentialsTest::setUp()
{
	m_cipherName = Environment::get("TEST_CIPHER_NAME", "aes256");
	m_params = CryptoParams::create(m_cipherName);
}

void CredentialsTest::testCrypt()
{
	const CipherKey &key = m_params.randomKey();
	SharedPtr<Cipher> cipher = m_factory.createCipher(key);

	///Pin Crypt
	SharedPtr<PinCredentials> pin01(new PinCredentials);
	pin01->setParams(m_params);
	pin01->setPin("pinkod01", cipher);

	CPPUNIT_ASSERT(pin01->pin(cipher) == "pinkod01");

	///Password Crypt
	SharedPtr<PasswordCredentials> pas01(new PasswordCredentials);
	pas01->setParams(m_params);
	pas01->setUsername("meno01", cipher);
	pas01->setPassword("heslo01", cipher);

	CPPUNIT_ASSERT(pas01->username(cipher) == "meno01");
	CPPUNIT_ASSERT(pas01->password(cipher) == "heslo01");
}

void CredentialsTest::testSave()
{
	AutoPtr<AbstractConfiguration> conf(new MapConfiguration);

	///Pin Save
	const DeviceID id01(0xa200000000000000UL);
	SharedPtr<PinCredentials> pin01(new PinCredentials);
	pin01->setParams(m_params);
	pin01->setRawPin("pinkod01");
	pin01->save(conf, id01);

	CPPUNIT_ASSERT_EQUAL(
		PinCredentials::TYPE,
		conf->getString("credentials.0xa200000000000000.type")
	);

	CPPUNIT_ASSERT_EQUAL(
		"pinkod01",
		conf->getString("credentials.0xa200000000000000.pin")
	);

	CPPUNIT_ASSERT_EQUAL(
		m_params.toString(),
		conf->getString("credentials.0xa200000000000000.params")
	);

	/// Password Save
	const DeviceID id02(0xa200000000000001UL);
	SharedPtr<PasswordCredentials> pas01(new PasswordCredentials);
	pas01->setParams(m_params);
	pas01->setRawUsername("meno01");
	pas01->setRawPassword("heslo01");
	pas01->save(conf, id02);

	CPPUNIT_ASSERT_EQUAL(
		PasswordCredentials::TYPE,
		conf->getString("credentials.0xa200000000000001.type")
	);

	CPPUNIT_ASSERT_EQUAL(
		"heslo01",
		conf->getString("credentials.0xa200000000000001.password")
	);

	CPPUNIT_ASSERT_EQUAL(
		"meno01",
		conf->getString("credentials.0xa200000000000001.username")
	);

	CPPUNIT_ASSERT_EQUAL(
		m_params.toString(),
		conf->getString("credentials.0xa200000000000001.params")
	);
}

void CredentialsTest::testLoad()
{
	AutoPtr<AbstractConfiguration> conf(new MapConfiguration);
	SharedPtr<Credentials> credential;

	/// Pin Load
	const DeviceID id01(0xa200000000000000UL);

	conf->setString("credentials.0xa200000000000000.pin", "pinkod01");
	conf->setString("credentials.0xa200000000000000.params", m_params.toString());

	credential = PinCredentials::create(
			conf->createView("credentials.0xa200000000000000"));

	SharedPtr<PinCredentials> pin01 = credential.cast<PinCredentials>();

	CPPUNIT_ASSERT_EQUAL(
		"pinkod01",
		conf->getString("credentials.0xa200000000000000.pin")
	);

	CPPUNIT_ASSERT_EQUAL(
		pin01->params().toString(),
		conf->getString("credentials.0xa200000000000000.params")
	);

	/// Password Load
	const DeviceID id02(0xa200000000000001UL);

	conf->setString("credentials.0xa200000000000001.username", "meno01");
	conf->setString("credentials.0xa200000000000001.password", "heslo01");
	conf->setString("credentials.0xa200000000000001.params", m_params.toString());

	credential = PasswordCredentials::create(
			conf->createView("credentials.0xa200000000000001"));

	SharedPtr<PasswordCredentials> pas01 = credential.cast<PasswordCredentials>();

	CPPUNIT_ASSERT_EQUAL(
		"heslo01",
		conf->getString("credentials.0xa200000000000001.password")
	);

	CPPUNIT_ASSERT_EQUAL(
		"meno01",
		conf->getString("credentials.0xa200000000000001.username")
	);

	CPPUNIT_ASSERT_EQUAL(
		pas01->params().toString(),
		conf->getString("credentials.0xa200000000000001.params")
	);
}

}
