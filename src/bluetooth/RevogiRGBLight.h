#pragma once

#include <Poco/SharedPtr.h>

#include "bluetooth/HciConnection.h"
#include "bluetooth/RevogiDevice.h"

namespace BeeeOn {

/**
 * @brief The class represents generic Revogi RGB light. It
 * allows to control the on/off, brightness and color module.
 *
 * The messages that are sent to the device consist of three
 * parts: the prefix, the message body and the suffix.
 * Characteristic for the suffix is that it contains a checksum
 * that is calculated for each command differently.
 */
class RevogiRGBLight: public RevogiDevice {
public:
	typedef Poco::SharedPtr<RevogiRGBLight> Ptr;

	RevogiRGBLight(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const std::string& productName,
		const std::list<ModuleType>& moduleTypes);
	~RevogiRGBLight();

protected:
	void modifyStatus(const double value, const HciConnection::Ptr conn);
	void modifyBrightness(
		const double value,
		const uint32_t rgb,
		const HciConnection::Ptr conn);
	void modifyColor(const double value, const HciConnection::Ptr conn);

	unsigned char brightnessFromPercents(const double percents) const;
	unsigned int brightnessToPercents(const double value) const;

	/**
	 * @brief Returns RGB contained in uint32_t. The components of RGB are
	 * retrieved from recived message contained actual setting of the light.
	 */
	uint32_t retrieveRGB(const std::vector<unsigned char>& values) const;

	uint32_t colorChecksum(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue) const;

	void prependHeader(std::vector<unsigned char>& payload) const override;
	void appendFooter(
		std::vector<unsigned char>& payload,
		const unsigned char checksum) const override;
};

}
