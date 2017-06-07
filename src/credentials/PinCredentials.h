#pragma once

#include "credentials/Credentials.h"

namespace Poco {
namespace Crypto {

class Cipher;

}
}

namespace BeeeOn {

class PinCredentials: public Credentials {
public:
	PinCredentials();
	~PinCredentials();

	static const std::string TYPE;

	void setPin(const std::string &pin, Poco::Crypto::Cipher *cipher);
	void setRawPin(const std::string &pin);

	std::string pin(Poco::Crypto::Cipher *cipher) const;

	void save(
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf,
		const DeviceID &device,
		const std::string &root = "credentials") const override;

	static Poco::SharedPtr<Credentials> create(
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf);

private:
	std::string m_pin;
};

}
