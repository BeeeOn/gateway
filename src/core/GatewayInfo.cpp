#include <string>

#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/RegularExpression.h>

#include "core/GatewayInfo.h"
#include "core/Version.h"
#include "di/Injectable.h"
#include "model/GatewayID.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GatewayInfo)
BEEEON_OBJECT_TEXT("certificatePath", &GatewayInfo::setCertPath)
BEEEON_OBJECT_TEXT("keyPath", &GatewayInfo::setKeyPath)
BEEEON_OBJECT_TEXT("gatewayID", &GatewayInfo::setGatewayID)
BEEEON_OBJECT_HOOK("done", &GatewayInfo::initialize)
BEEEON_OBJECT_END(BeeeOn, GatewayInfo)

using namespace BeeeOn;

using std::string;

using Poco::Net::X509Certificate;
using Poco::Crypto::RSAKey;

GatewayInfo::GatewayInfo()
{
}

void GatewayInfo::setCertPath(const string &path)
{
	m_certPath = path;
}

void GatewayInfo::setKeyPath(const string &path)
{
	m_keyPath = path;
}

void GatewayInfo::setGatewayID(const string &gatewayID)
{
	m_gatewayID = GatewayID::parse(gatewayID);
	poco_warning(logger(),
		"gatewayID was set directly, this should be done only in case of debugging");
}

GatewayID GatewayInfo::gatewayID() const
{
	if (m_gatewayID.isNull())
		throw Poco::NullPointerException("no gateway ID set");

	return m_gatewayID.value();
}

void GatewayInfo::initialize()
{
	if (!m_keyPath.empty())
		loadPrivateKey();

	if (!m_certPath.empty())
		loadCertificate();

	poco_notice(logger(), "gateway ID " + gatewayID().toString());
}

string GatewayInfo::version()
{
	return BeeeOn::gitVersion();
}

Poco::SharedPtr<X509Certificate> GatewayInfo::certificate() const
{
	return m_certificate;
}

void GatewayInfo::loadCertificate()
{
	m_certificate.assign(new X509Certificate(m_certPath));
	string commonName = m_certificate->commonName();
	string stringID = "";
	Poco::RegularExpression numberPattern("[0-9]+");

	poco_debug(logger() ,"loaded certificate with common name: " + commonName);

	if (numberPattern.extract(commonName, stringID) == 1) {
		if (!m_gatewayID.isNull()) {
			poco_warning(logger(),
				"overriding directly set gateway ID from certificate: "
				+ stringID);
		}

		m_gatewayID = GatewayID::parse(stringID);
	}
	else {
		m_certificate.assign(NULL);
		throw Poco::Exception("could not extract gateway ID from certificate");
	}
}

void GatewayInfo::loadPrivateKey()
{
	m_privateKey.assign(new RSAKey("", m_keyPath));
}
