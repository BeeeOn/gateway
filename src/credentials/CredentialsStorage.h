#pragma once

#include <functional>
#include <map>

#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/StringTokenizer.h>
#include <Poco/RWLock.h>

#include "credentials/Credentials.h"
#include "model/DeviceID.h"
#include "util/Loggable.h"

namespace BeeeOn {

class CredentialsStorage : protected Loggable {
public:
	/**
	* Factory method to construct a Credentials instance from the
	* AbstractConfiguration. The given configuration is a view into
	* the last level of the credentials key.
	*
	* Example:
	*
	* Configuration:
	*
	* credentials.0xff0123456789abcd.type = sometype
	* credentials.0xff0123456789abcd.value = X
	*
	* is send to function CredentialsStorage::load and
	* transformed into:
	*
	* type = sometype
	* value = X
	*
	* The corresponding CredentialsFactory function is selected from
	* CredentialsStorage::m_factory based on the "type". It creates
	* the appropriate Credentials instance from the transformed
	* configuration.
	*/
	typedef std::function<
		Poco::SharedPtr<Credentials> (Poco::AutoPtr<Poco::Util::AbstractConfiguration>)>
		CredentialsFactory;

	CredentialsStorage();
	CredentialsStorage(const std::map<std::string, CredentialsFactory> &factory);

	~CredentialsStorage();

	Poco::SharedPtr<Credentials> find(const DeviceID &ID);

	virtual void insertOrUpdate(
		const DeviceID &device,
		const Poco::SharedPtr<Credentials> credentials);

	virtual void remove(const DeviceID &device);
	virtual void clear();

	void save(
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf,
		const std::string &root = "credentials") const;

	void load(
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> rootConf,
		const std::string &root = "credentials");

protected:
	Poco::RWLock &lock() const;

	void insertOrUpdateUnlocked(
		const DeviceID &device,
		const Poco::SharedPtr<Credentials> credentials);

	void removeUnlocked(const DeviceID &device);
	void clearUnlocked();

private:
	Poco::SharedPtr<Credentials> createCredential(
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf);

	std::map<DeviceID, Poco::SharedPtr<Credentials>> m_credentialsMap;
	std::map<std::string, CredentialsFactory> m_factory;
	mutable Poco::RWLock m_lock;
};

}
