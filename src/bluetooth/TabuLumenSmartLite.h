#pragma once

#include <Poco/SharedPtr.h>
#include <Poco/UUID.h>

#include "bluetooth/BLESmartDevice.h"
#include "util/ColorBrightness.h"

namespace BeeeOn {

/**
 * @brief The class represents Tabu Lumen TL 100S Smart Light. It allows
 * to control all its modules.
 */
class TabuLumenSmartLite : public BLESmartDevice {
public:
	typedef Poco::SharedPtr<TabuLumenSmartLite> Ptr;

private:
	/**
	 * UUID of characteristics to modify device status and to authorize.
	 */
	static const Poco::UUID WRITE_VALUES;
	static const std::vector<uint8_t> ADD_KEY;
	static const std::vector<uint8_t> XOR_KEY;
	static const std::string LIGHT_NAME;
	static const std::string VENDOR_NAME;

	enum Command {
		LOGIN = 0x08,
		OFF = 0x00,
		ON_ACTION = 0x01
	};

public:
	TabuLumenSmartLite(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const HciInterface::Ptr hci);
	~TabuLumenSmartLite();

	std::list<ModuleType> moduleTypes() const override;
	std::string productName() const override;
	std::string vendor() const override;

	void requestModifyState(
		const ModuleID& moduleID,
		const double value) override;

	static bool match(const std::string& modelID);

protected:
	void modifyStatus(const int64_t value, const HciInterface::Ptr hci);
	void modifyBrightness(const int64_t value, const HciInterface::Ptr hci);
	void modifyColor(const int64_t value, const HciInterface::Ptr hci);

	void writeData(
		const std::vector<unsigned char>& data,
		const HciInterface::Ptr hci) const;

	/**
	 * @brief Returns authorization message that must be sent to the device
	 * after connection so that it can be manipulated with the device.
	 */
	std::vector<uint8_t> authorizationMessage() const;

	/**
	 * Encrypting and decrypting message is taken from
	 * https://github.com/mrquincle/luminosi/blob/master/web.js
	 */
	void encryptMessage(std::vector<uint8_t> &data) const;
	void decryptMessage(std::vector<uint8_t> &data) const;

private:
	ColorBrightness m_colorBrightness;
};

}
