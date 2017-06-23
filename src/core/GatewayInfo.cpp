#include <string>

#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/RegularExpression.h>

#include "core/GatewayInfo.h"
#include "core/Version.h"
#include "di/Injectable.h"
#include "model/GatewayID.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GatewayInfo)
BEEEON_OBJECT_TEXT("loadCertificate", &GatewayInfo::loadCertificate)
BEEEON_OBJECT_TEXT("loadPrivateKey", &GatewayInfo::loadPrivateKey)
BEEEON_OBJECT_END(BeeeOn, GatewayInfo)

using namespace BeeeOn;

using std::string;

using Poco::Net::X509Certificate;
using Poco::Crypto::RSAKey;

GatewayInfo::GatewayInfo()
{
}

GatewayID GatewayInfo::gatewayID() const
{
	if (m_certificate.isNull())
		throw Poco::Exception(string("could not retrieve gatewayID,")
				+ string("certificate needs to be loaded first"));

	return m_gatewayID;
}

string GatewayInfo::version()
{
	return GIT_ID;
}

Poco::SharedPtr<X509Certificate> GatewayInfo::certificate() const
{
	return m_certificate;
}

void GatewayInfo::loadCertificate(const string &filePath)
{
	m_certificate.assign(new X509Certificate(filePath));
	string commonName = m_certificate->commonName();
	string stringID = "";
	Poco::RegularExpression numberPattern("[0-9]+");

	poco_debug(logger() ,"loaded certificate with common name: " + commonName);

	if (numberPattern.extract(commonName, stringID) == 1) {
		m_gatewayID = GatewayID::parse(stringID);
	}
	else {
		m_certificate.assign(NULL);
		throw Poco::Exception("could not extract gateway ID from certificate");
	}
}

void GatewayInfo::loadPrivateKey(const std::string &filePath)
{
	m_privateKey.assign(new RSAKey("", filePath));
}
