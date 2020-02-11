#pragma once

#include <string>

#include <Poco/Timestamp.h>

namespace BeeeOn {

/**
 * @brief The class stores a statistic information about
 * device maintained by FHEM server. According to these
 * information FHEMClient generates events.
 */
class FHEMDeviceInfo {
public:
	FHEMDeviceInfo(
		const std::string& dev,
		const uint32_t protRcv,
		const uint32_t protSnd,
		const Poco::Timestamp& lastRcv);

	std::string dev() const;

	uint32_t protRcv() const;
	void setProtRcv(const uint32_t protRcv);

	uint32_t protSnd() const;
	void setProtSnd(const uint32_t protSnd);

	Poco::Timestamp lastRcv() const;
	void setLastRcv(const Poco::Timestamp& lastRcv);

private:
	std::string m_dev;
	uint32_t m_protRcv;
	uint32_t m_protSnd;
	Poco::Timestamp m_lastRcv;
};

}
