#pragma once

#include <string>

#include "credentials/CredentialsStorage.h"
#include "loop/Tool.h"
#include "util/CryptoConfig.h"

namespace BeeeOn {

/**
 * @brief Standalone tool that can be used to manipulate the credentials
 * storage directly. It parses the given command and performs the given
 * action.
 *
 * Supported commands:
 *
 * - clear
 * - remove
 * - set <device-id> password <password>
 * - set <device-id> password <username> <password>
 * - set <device-id> pin <pin>
 */
class CredentialsTool : public Tool {
public:
	CredentialsTool();

	void setCryptoConfig(Poco::SharedPtr<CryptoConfig> config);
	void setStorage(CredentialsStorage::Ptr storage);

protected:
	void main(
		ConsoleSession &session,
		const std::vector<std::string> &args) override;

	void actionClear();
	void actionRemove(const std::vector<std::string> &args);
	void actionSet(const std::vector<std::string> &args);

private:
	CredentialsStorage::Ptr m_storage;
	Poco::SharedPtr<CryptoConfig> m_cryptoConfig;
};

}
