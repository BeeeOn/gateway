#include "credentials/CredentialsStorage.h"
#include "credentials/PasswordCredentials.h"
#include "credentials/PinCredentials.h"

using namespace BeeeOn;
using namespace std;
using namespace Poco;
using namespace Poco::Util;

CredentialsStorage::CredentialsStorage()
{
	m_factory[PasswordCredentials::TYPE] = &PasswordCredentials::create;
	m_factory[PinCredentials::TYPE] = &PinCredentials::create;
}

CredentialsStorage::CredentialsStorage(
		const map<string, CredentialsFactory> &factory):
	m_factory(factory)
{
}

CredentialsStorage::~CredentialsStorage()
{
}

SharedPtr<Credentials> CredentialsStorage::find(const DeviceID &ID)
{
	RWLock::ScopedReadLock guard(lock());
	auto search = m_credentialsMap.find(ID);

	if (search != m_credentialsMap.end())
		return search->second;
	else
		return NULL;
}

void CredentialsStorage::insertOrUpdate(
		const DeviceID &device,
		const SharedPtr<Credentials> credentials)
{
	RWLock::ScopedWriteLock guard(lock());
	insertOrUpdateUnlocked(device, credentials);
}

void CredentialsStorage::insertOrUpdateUnlocked(
		const DeviceID &device,
		const SharedPtr<Credentials> credentials)
{
	m_credentialsMap[device] = credentials;
}

void CredentialsStorage::remove(const DeviceID &device)
{
	RWLock::ScopedWriteLock guard(lock());
	removeUnlocked(device);
}

void CredentialsStorage::removeUnlocked(const DeviceID &device)
{
	m_credentialsMap.erase(device);
}

void CredentialsStorage::clear()
{
	RWLock::ScopedWriteLock guard(m_lock);
	clearUnlocked();
}

void CredentialsStorage::clearUnlocked()
{
	m_credentialsMap.clear();
}

void CredentialsStorage::save(
		AutoPtr<AbstractConfiguration> conf,
		const std::string &root) const
{
	for (auto const& itr : m_credentialsMap)
		itr.second->save(conf, itr.first, root);
}

SharedPtr<Credentials> CredentialsStorage::createCredential(
		AutoPtr<AbstractConfiguration> conf)
{
	string type = conf->getString("type");

	if (m_factory.find(type) == m_factory.end())
		throw InvalidArgumentException("unrecognized credential type: " + type);

	return m_factory[type](conf);
}

void CredentialsStorage::load(
		AutoPtr<AbstractConfiguration> rootConf,
		const std::string &root)
{
	RWLock::ScopedWriteLock guard(lock());

	AutoPtr<AbstractConfiguration> conf = rootConf->createView(root);

	AbstractConfiguration::Keys keys;
	conf->keys(keys);

	for (auto itr : keys) {
		DeviceID id;

		try {
			id = DeviceID::parse(itr);
		} catch (const Exception &e) {
			logger().warning("expected DeviceID, got: " + itr);
			continue;
		}

		try {
			insertOrUpdateUnlocked(id, createCredential(conf->createView(itr)));
		} catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
		}
	}
}

Poco::RWLock &CredentialsStorage::lock() const
{
	return m_lock;
}
