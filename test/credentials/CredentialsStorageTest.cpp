#include <cppunit/extensions/HelperMacros.h>
#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/Util/MapConfiguration.h>

#include "cppunit/BetterAssert.h"

#include "credentials/Credentials.h"
#include "credentials/CredentialsStorage.h"

using namespace std;
using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Util;
using namespace Poco::Crypto;

namespace BeeeOn {

class TestingCredentials : public Credentials {
public:
	void setName(const std::string &name)
	{
		m_name = name;
	}

	std::string name()
	{
		return m_name;
	}

	void save(
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf,
		const DeviceID &device,
		const std::string &root) const
	{
		conf->setString(root + "." + device.toString() + "." + "type", "test");
		conf->setString(root + "." + device.toString() + "." + "name", m_name);
	}

	static SharedPtr<Credentials> create(AutoPtr<AbstractConfiguration> conf)
	{
		SharedPtr<TestingCredentials> ptr(new TestingCredentials);
		ptr->setName(conf->getString("name"));
		return ptr;
	}

private:
        std::string m_name;
};


class CredentialsStorageTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(CredentialsStorageTest);
	CPPUNIT_TEST(testInsertUpdateFindRemove);
	CPPUNIT_TEST(testSave);
	CPPUNIT_TEST(testLoad);
	CPPUNIT_TEST_SUITE_END();

public:
	void testInsertUpdateFindRemove();
	void testSave();
	void testLoad();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CredentialsStorageTest);

void CredentialsStorageTest::testInsertUpdateFindRemove()
{
	CredentialsStorage storage;

	/// insert + find
	SharedPtr<TestingCredentials> test01(new TestingCredentials);
	test01->setName("meno01");

	SharedPtr<TestingCredentials> test02(new TestingCredentials);
	test02->setName("meno02");

	const DeviceID id01(0xa200000000000000UL);
	const DeviceID id02(0xa200000000000001UL);
	const DeviceID id03(0xa200000000000002UL);

	storage.insertOrUpdate(id01, test01);
	storage.insertOrUpdate(id02, test02);

	CPPUNIT_ASSERT(!storage.find(id01).isNull());
	CPPUNIT_ASSERT(!storage.find(id02).isNull());
	CPPUNIT_ASSERT(storage.find(id03).isNull());

	CPPUNIT_ASSERT(!storage.find(id01).cast<TestingCredentials>().isNull());
	CPPUNIT_ASSERT(!storage.find(id02).cast<TestingCredentials>().isNull());

	CPPUNIT_ASSERT_EQUAL(
		"meno01",
		storage.find(id01).cast<TestingCredentials>()->name()
	);

	CPPUNIT_ASSERT_EQUAL(
		"meno02",
		storage.find(id02).cast<TestingCredentials>()->name()
	);

	/// update
	SharedPtr<TestingCredentials> test03(new TestingCredentials);
	test03->setName("meno02_update");

	storage.insertOrUpdate(id02, test03);

	CPPUNIT_ASSERT(!storage.find(id02).isNull());
	CPPUNIT_ASSERT(!storage.find(id02).cast<TestingCredentials>().isNull());

	CPPUNIT_ASSERT_EQUAL(
		string("meno02_update"),
		storage.find(id02).cast<TestingCredentials>()->name()
	);

	/// remove
	storage.remove(id01);
	CPPUNIT_ASSERT(storage.find(id01).isNull());
}

void CredentialsStorageTest::testSave()
{
	CredentialsStorage storage;

	SharedPtr<Credentials> credential;
	SharedPtr<TestingCredentials> testCredential;

	SharedPtr<TestingCredentials> test01(new TestingCredentials);
	test01->setName("meno01");

	SharedPtr<TestingCredentials> test02(new TestingCredentials);
	test02->setName("meno02");

	const DeviceID id01(0xa200000000000000UL);
	const DeviceID id02(0xa200000000000001UL);

	storage.insertOrUpdate(id01, test01);
	storage.insertOrUpdate(id02, test02);

	AutoPtr<AbstractConfiguration> conf(new MapConfiguration);

	storage.save(conf);

	CPPUNIT_ASSERT_EQUAL(
		"meno01",
		conf->getString("credentials.0xa200000000000000.name")
	);

	CPPUNIT_ASSERT_EQUAL(
		"test",
		conf->getString("credentials.0xa200000000000000.type")
	);

	CPPUNIT_ASSERT_EQUAL(
		"meno02",
		conf->getString("credentials.0xa200000000000001.name")
	);

	CPPUNIT_ASSERT_EQUAL(
		"test",
		conf->getString("credentials.0xa200000000000001.type")
	);

}

void CredentialsStorageTest::testLoad()
{
	AutoPtr<AbstractConfiguration> conf(new MapConfiguration);

	const DeviceID id01(0xa200000000000000UL);
	const DeviceID id02(0xa200000000000001UL);

	conf->setString("credentials.0xa200000000000000.name", "meno01");
	conf->setString("credentials.0xa200000000000000.type", "test");

	conf->setString("credentials.0xa200000000000001.name", "meno02");
	conf->setString("credentials.0xa200000000000001.type", "test");

	map<string, CredentialsStorage::CredentialsFactory> factory;
	factory["test"] = &TestingCredentials::create;

	CredentialsStorage storage(factory);

	storage.load(conf);

	CPPUNIT_ASSERT(!storage.find(id01).isNull());
	CPPUNIT_ASSERT(!storage.find(id02).isNull());

	CPPUNIT_ASSERT(!storage.find(id01).cast<TestingCredentials>().isNull());
	CPPUNIT_ASSERT(!storage.find(id02).cast<TestingCredentials>().isNull());

	CPPUNIT_ASSERT_EQUAL(
		"meno01",
		storage.find(id01).cast<TestingCredentials>()->name()
	);

	CPPUNIT_ASSERT_EQUAL(
		"meno02",
		storage.find(id02).cast<TestingCredentials>()->name()
	);

}

}
