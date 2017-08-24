#ifndef BEEEON_HCI_INTERFACE_H
#define BEEEON_HCI_INTERFACE_H

#include <list>
#include <string>
#include <utility>
#include <vector>

#include <Poco/Mutex.h>

#include "net/MACAddress.h"

namespace BeeeOn {

class HciInterface {
public:
	HciInterface(const std::string &name);

	/**
	 * Try to set hci interface up.
	 * The root priviledges of the system might be required.
	 * @throws IOException in case of a failure
	 */
	void up() const;

	/**
	 * Check state of device with MACAddress.
	 * @return true if the device was detected or false
	 * @throws IOException when the detection fails for some reason
	 */
	bool detect(const MACAddress &address) const;

	/**
	 * Full scan of bluetooth network.
	 * This can find only visible devices.
	 * @return list of MAC addresses with names
	 */
	std::list<std::pair<std::string, MACAddress>> scan() const;

private:
	/**
	 * Command execution utility.
	 * @param command - program.
	 * @param args    - vector of agruments for program
	 * @param result  - stdout from program in terminal
	 * @return exit_code
	 *   return 0     => OK
	 *   return n > 0 => Some error
	 */
	int exec(const std::string &command, const std::vector<std::string> &args,
		std::string &output) const;

	/**
	 * Parse and process the stdout message from hcitool scan.
	 * Example stdout:
	   Scanning ...
		   AA:BB:CC:DD:EE:FF	Device_name
	 */
	std::list<std::pair<std::string, MACAddress>> parseScan(const std::string &scan) const;

	/**
	 * Distribution of string into the vector by divide string
	 */
	std::vector<std::string> split(const std::string &input, const std::string &div) const;

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

private:
	mutable Poco::Mutex m_mutexExec;
	std::string m_name;
};

}

#endif // BEEEON_HCI_INTERFACE_H
