#ifndef BEEEON_BLUEZ_HCI_INTERFACE_H
#define BEEEON_BLUEZ_HCI_INTERFACE_H

#include <utility>
#include <vector>

#include <Poco/Mutex.h>

#include "bluetooth/HciInterface.h"
#include "util/Loggable.h"

namespace BeeeOn {

class BluezHciInterface : public HciInterface, Loggable {
public:
	BluezHciInterface(const std::string &name);

	void up() const override;
	void reset() const override;
	bool detect(const MACAddress &address) const override;
	std::map<MACAddress, std::string> scan() const override;
	std::map<MACAddress, std::string> lescan(
			const Poco::Timespan &seconds) const override;
	HciInfo info() const override;

protected:
	/**
	 * Find device name in le_advertising_info struct
	 * @return name of found device or an empty string otherwise
	 */
	static std::string parseLEName(uint8_t *eir, size_t length);

private:
	/**
	 * Open HCI socket to be able to ioctl() about HCI interfaces.
	 * @throws IOException on failure
	 */
	int hciSocket() const;

	/**
	 * Find index of the HCI interface by the given name.
	 * @throws NotFoundException when not found
	 * @see HciInterface::findHci(int, std::string)
	 */
	int findHci(const std::string &name) const;

	/**
	 * Find index of the HCI interface by the given name.
	 * Use the given HCI socket for this operation.
	 * @throws NotFoundException when not found
	 * @see HciInterface::hciSocket()
	 */
	int findHci(int sock, const std::string &name) const;

	/**
	 * Read data from socket about devices in BLE network
	 * and get MACAddress and name of found devices when it's available.
	 * @return false when no more events should be processed
	 */
	bool processNextEvent(const int &fd, std::map<MACAddress, std::string> &devices) const;

	/**
	 * Read available information about devices in bluetooth low energy
	 * network. Sets the parameters of the socket for low energy and
	 * after reading restores the original settings. Because of read()
	 * there is used poll() for stop reading after defined time (timeout).
	 * Found devices are stored to map devices.
	 * @throws IOException when something wrong with socket
	 */
	std::map<MACAddress, std::string> listLE(
		const int sock, const Poco::Timespan &seconds) const;

private:
	std::string m_name;
};

}

#endif // BEEEON_BLUEZ_HCI_INTERFACE_H
