#include <Poco/Crypto/Cipher.h>

#include "credentials/PasswordCredentials.h"

using namespace BeeeOn;
using namespace std;
using namespace Poco;
using namespace Poco::Crypto;
using namespace Poco::Util;

PasswordCredentials::PasswordCredentials()
{
}

PasswordCredentials::~PasswordCredentials()
{
}

const string PasswordCredentials::TYPE = "password";

string PasswordCredentials::username(Cipher *cipher) const
{
	return cipher->decryptString(m_username, Cipher::ENC_BASE64);
}

string PasswordCredentials::password(Cipher *cipher) const
{
	return cipher->decryptString(m_password, Cipher::ENC_BASE64);
}

void PasswordCredentials::setUsername(const string &username, Cipher *cipher)
{
	m_username = cipher->encryptString(username, Cipher::ENC_BASE64);
}

void PasswordCredentials::setPassword(const string &password, Cipher *cipher)
{
	m_password = cipher->encryptString(password, Cipher::ENC_BASE64);
}

void PasswordCredentials::setRawUsername(const string &username)
{
	m_username = username;
}

void PasswordCredentials::setRawPassword(const string &password)
{
	m_password = password;
}

void PasswordCredentials::save(
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf,
	const DeviceID &device,
	const std::string &root) const
{
	conf->setString(makeConfString(device, "type", root), TYPE);
	conf->setString(makeConfString(device, "params", root), m_params.toString());
	conf->setString(makeConfString(device, "username", root), m_username);
	conf->setString(makeConfString(device, "password", root), m_password);
}

SharedPtr<Credentials> PasswordCredentials::create(
	AutoPtr<AbstractConfiguration> conf)
{
	SharedPtr<PasswordCredentials> ptr(new PasswordCredentials);
	ptr->setParams(CryptoParams::parse(conf->getString("params")));
	ptr->m_username = conf->getString("username");
	ptr->m_password = conf->getString("password");
	return ptr;
}
