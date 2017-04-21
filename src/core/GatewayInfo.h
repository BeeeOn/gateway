#pragma once

#include <string>

#include <Poco/Crypto/RSAKey.h>
#include <Poco/Net/X509Certificate.h>
#include <Poco/SharedPtr.h>

#include "model/GatewayID.h"
#include "util/Loggable.h"

namespace BeeeOn {

/*
 * Class for storing basic information about gatway.
 * */
class GatewayInfo : public Loggable {
public:
	/*
	 * Creates an uninitialized instance.
	 * Needs to be initialized first using
	 * "loadCertificate()".
	 * */
	GatewayInfo();

	GatewayID gatewayID() const;
	static std::string version();
	Poco::SharedPtr<Poco::Net::X509Certificate> certificate() const;

	/*
	 * Loads and stores X509 certificate from file located at @filePath.
	 * Also gets and stores gatewayID from common name of the certificate.
	 * */
	void loadCertificate(const std::string &filePath);

	/*
	 * Loads and stores RSA private key from file located at @filePath.
	 * */
	void loadPrivateKey(const std::string &filePath);

private:
	GatewayID m_gatewayID;
	Poco::SharedPtr<Poco::Net::X509Certificate> m_certificate;
	Poco::SharedPtr<Poco::Crypto::RSAKey> m_privateKey;

};

}
