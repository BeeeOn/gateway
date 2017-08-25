#pragma once

#include <string>

#include <Poco/Crypto/RSAKey.h>
#include <Poco/Net/X509Certificate.h>
#include <Poco/Nullable.h>
#include <Poco/SharedPtr.h>

#include "model/GatewayID.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Class for storing basic information about gateway.
 */
class GatewayInfo : public Loggable {
public:
	/**
	 * Creates an uninitialized instance.
	 */
	GatewayInfo();

	/**
	 * GatewayID can be set directly for debugging purposes.
	 */
	void setGatewayID(const std::string &gatewayID);
	GatewayID gatewayID() const;

	static std::string version();
	Poco::SharedPtr<Poco::Net::X509Certificate> certificate() const;

	void setCertPath(const std::string &path);
	void setKeyPath(const std::string &path);

	void initialize();

protected:
	/**
	 * Loads and stores X509 certificate from file.
	 * Also gets and stores gatewayID from common name of the certificate.
	 */
	void loadCertificate();

	/**
	 * Loads and stores RSA private key from file.
	 */
	void loadPrivateKey();

private:
	Poco::Nullable<GatewayID> m_gatewayID;
	Poco::SharedPtr<Poco::Net::X509Certificate> m_certificate;
	Poco::SharedPtr<Poco::Crypto::RSAKey> m_privateKey;

	std::string m_certPath;
	std::string m_keyPath;

};

}
