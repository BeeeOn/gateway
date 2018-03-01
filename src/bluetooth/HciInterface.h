#ifndef BEEEON_HCI_INTERFACE_H
#define BEEEON_HCI_INTERFACE_H

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <Poco/Mutex.h>
#include <Poco/Timespan.h>

#include "bluetooth/HciInfo.h"
#include "net/MACAddress.h"
#include "util/Loggable.h"

namespace BeeeOn {

class HciInterface : Loggable {
public:
	HciInterface(const std::string &name);

	/**
	 * Try to set hci interface up.
	 * The root priviledges of the system might be required.
	 * @throws IOException in case of a failure
	 */
	void up() const;

	/**
	 * Reset hci interface - turn down & up.
	 * @throws IOException in case of a failure
	 */
	void reset() const;

	/**
	 * Check state of device with MACAddress.
	 * @return true if the device was detected or false
	 * @throws IOException when the detection fails for some reason
	 */
	bool detect(const MACAddress &address) const;

	/**
	 * Full scan of bluetooth network.
	 * This can find only visible devices.
	 * @return map of MAC addresses with names
	 */
	std::map<MACAddress, std::string> scan() const;

	/**
	 * Full scan of low energy bluetooth network.
	 * Sets parameters for low energy scan and open socket.
	 * @return list of MAC addresses with names
	 * @throws IOException when the detection fails for some reason
	 */
	std::map<MACAddress, std::string> lescan(const Poco::Timespan &seconds) const;

	/**
	 * Read information about the iterface.
	 */
	HciInfo info() const;

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

#endif // BEEEON_HCI_INTERFACE_H
