#include <Poco/Crypto/Cipher.h>

#include "credentials/PinCredentials.h"

using namespace BeeeOn;
using namespace std;
using namespace Poco;
using namespace Poco::Crypto;
using namespace Poco::Util;

PinCredentials::PinCredentials()
{
}

PinCredentials::~PinCredentials()
{
}

const string PinCredentials::TYPE = "pin";

void PinCredentials::setPin(const string &pin, Cipher *cipher)
{
	m_pin = cipher->encryptString(pin, Cipher::ENC_BASE64);
}

void PinCredentials::setRawPin(const string &pin)
{
	m_pin = pin;
}

string PinCredentials::pin(Cipher *cipher) const
{
	return cipher->decryptString(m_pin, Cipher::ENC_BASE64);
}

void PinCredentials::save(
	AutoPtr<AbstractConfiguration> conf,
	const DeviceID &device,
	const string &root) const
{
	conf->setString(makeConfString(device, "type", root), TYPE);
	conf->setString(makeConfString(device, "params", root), m_params.toString());
	conf->setString(makeConfString(device, "pin", root), m_pin);
}

SharedPtr<Credentials> PinCredentials::create(
	AutoPtr<AbstractConfiguration> conf)
{
	SharedPtr<PinCredentials> ptr(new PinCredentials);
	ptr->setParams(CryptoParams::parse(conf->getString("params")));
	ptr->m_pin = conf->getString("pin");
	return ptr;
}
