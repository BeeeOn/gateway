#include "credentials/Credentials.h"

using namespace BeeeOn;
using namespace std;

Credentials::~Credentials()
{
}

void Credentials::setParams(const CryptoParams &params)
{
	m_params = params;
}

CryptoParams Credentials::params() const
{
	return m_params;
}


string Credentials::makeConfString(
		const DeviceID &device,
		const string &atribute,
		const std::string &root) const
{
	string str = root;
	str.append("." + device.toString());
	str.append("." + atribute);
	return str;
}
