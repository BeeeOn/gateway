#pragma once

#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/SharedPtr.h>

#include "util/CryptoParams.h"
#include "model/DeviceID.h"

namespace BeeeOn {

class Credentials {
public:
	virtual ~Credentials();

	void setParams(const CryptoParams &params);
	CryptoParams params() const;

	virtual void save(
			Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf,
			const DeviceID &device,
			const std::string &root) const = 0;

protected:
	/**
	 * Creates a configuration key in format: "<root>.<device>.<atribute>".
	 */
	std::string makeConfString(
			const DeviceID &device,
			const std::string &atribute,
			const std::string &root) const;

	CryptoParams m_params;
};

}
