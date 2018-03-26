#ifndef BEEEON_PHILIPS_HUE_BULB_INFO_H
#define BEEEON_PHILIPS_HUE_BULB_INFO_H

#include <map>
#include <string>

namespace BeeeOn {

/**
 * Provides information about Philips Hue light.
 */
class PhilipsHueBulbInfo {
public:
	PhilipsHueBulbInfo();

	std::map<std::string, std::string> modules() const;
	bool reachable() const;
	std::string type() const;
	std::string name() const;
	std::string modelId() const;
	std::string manufacturerName() const;
	std::string uniqueId() const;
	std::string swVersion() const;

	static PhilipsHueBulbInfo buildBulbInfo(
		const std::string& response);

private:
	std::map<std::string, std::string> m_modules;
	bool m_reachable;
	std::string m_type;
	std::string m_name;
	std::string m_modelId;
	std::string m_manufacturerName;
	std::string m_uniqueId;
	std::string m_swVersion;
};

}

#endif
