#pragma once

#include <string>

#include "credentials/Credentials.h"

namespace Poco {
namespace Crypto {

class Cipher;

}
}

namespace BeeeOn {

class PasswordCredentials: public Credentials {
public:
	PasswordCredentials();
	~PasswordCredentials();

	static const std::string TYPE;

	std::string username(Poco::Crypto::Cipher *cipher) const;
	std::string password(Poco::Crypto::Cipher *cipher) const;

	void setUsername(const std::string &username, Poco::Crypto::Cipher *cipher);
	void setPassword(const std::string &password, Poco::Crypto::Cipher *cipher);

	void setRawUsername(const std::string &username);
	void setRawPassword(const std::string &password);

	void save(
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf,
		const DeviceID &device,
		const std::string &root = "credentials") const override;

	static Poco::SharedPtr<Credentials> create(
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf);

private:
	std::string m_username;
	std::string m_password;
};

}
