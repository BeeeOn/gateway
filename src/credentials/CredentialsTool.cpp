#include <iostream>

#include <Poco/AutoPtr.h>
#include <Poco/Exception.h>

#include <Poco/Crypto/Cipher.h>
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/CipherKey.h>

#include "di/Injectable.h"
#include "credentials/CredentialsTool.h"
#include "credentials/PasswordCredentials.h"
#include "credentials/PinCredentials.h"
#include "model/DeviceID.h"

BEEEON_OBJECT_BEGIN(BeeeOn, CredentialsTool)
BEEEON_OBJECT_CASTABLE(StoppableLoop)
BEEEON_OBJECT_PROPERTY("cryptoConfig", &CredentialsTool::setCryptoConfig)
BEEEON_OBJECT_PROPERTY("storage", &CredentialsTool::setStorage)
BEEEON_OBJECT_PROPERTY("cmd", &CredentialsTool::setCommand)
BEEEON_OBJECT_PROPERTY("console", &CredentialsTool::setConsole)
BEEEON_OBJECT_END(BeeeOn, CredentialsTool)

using namespace std;
using namespace Poco;
using namespace Poco::Crypto;
using namespace BeeeOn;

CredentialsTool::CredentialsTool()
{
}

void CredentialsTool::setCryptoConfig(SharedPtr<CryptoConfig> config)
{
	m_cryptoConfig = config;
}

void CredentialsTool::setStorage(CredentialsStorage::Ptr storage)
{
	m_storage = storage;
}

void CredentialsTool::actionClear()
{
	m_storage->clear();
}

void CredentialsTool::actionRemove(const std::vector<std::string> &args)
{
	if (args.size() != 1)
		throw InvalidArgumentException("missing argument <device-id>");

	m_storage->remove(DeviceID::parse(args[0]));
}

void CredentialsTool::actionSet(const std::vector<std::string> &args)
{
	if (args.size() < 1)
		throw InvalidArgumentException("missing argument <device-id>");

	const auto &id = DeviceID::parse(args[0]);

	if (args.size() < 2)
		throw InvalidArgumentException("missing argument <type>");

	const auto params = m_cryptoConfig->deriveParams();
	CipherKey key = m_cryptoConfig->createKey(params);
	AutoPtr<Cipher> cipher = CipherFactory::defaultFactory().createCipher(key);

	if (args[1] == "password") {
		SharedPtr<PasswordCredentials> cred = new PasswordCredentials;
		cred->setParams(params);

		if (args.size() < 3) {
			throw InvalidArgumentException(
				"missing arguments <password> or <username> <password>");
		}
		else if (args.size() == 3) {
			cred->setUsername("", cipher);
			cred->setPassword(args[2], cipher);
		}
		else if (args.size() == 4) {
			cred->setUsername(args[2], cipher);
			cred->setPassword(args[3], cipher);
		}
		else {
			throw InvalidArgumentException("too many arguments");
		}

		m_storage->insertOrUpdate(id, cred);
	}
	else if (args[1] == "pin") {
		SharedPtr<PinCredentials> cred = new PinCredentials;
		cred->setParams(params);

		if (args.size() < 3) {
			throw InvalidArgumentException("missing argument <pin>");
		}

		cred->setPin(args[2], cipher);

		m_storage->insertOrUpdate(id, cred);
	}
	else {
		throw InvalidArgumentException(
			"unrecognized credentials type: " + args[1]);
	}
}

void CredentialsTool::main(
	ConsoleSession &,
	const vector<string> &args)
{
	if (args.empty())
		return;

	if (args[0] == "clear")
		actionClear();
	else if (args[0] == "remove")
		actionRemove({++args.begin(), args.end()});
	else if (args[0] == "set")
		actionSet({++args.begin(), args.end()});
	else
		throw InvalidArgumentException("unrecognized command: " + args[0]);
}
