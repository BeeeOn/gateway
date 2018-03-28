#pragma once

#include <map>
#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "bluetooth/HciInfo.h"
#include "net/MACAddress.h"

namespace BeeeOn {

class HciInterface {
public:
	typedef Poco::SharedPtr<HciInterface> Ptr;

	virtual ~HciInterface();

	/**
	 * Try to set hci interface up.
	 * The root priviledges of the system might be required.
	 * @throws IOException in case of a failure
	 */
	virtual void up() const = 0;

	/**
	 * Reset hci interface - turn down & up.
	 * @throws IOException in case of a failure
	 */
	virtual void reset() const = 0;

	/**
	 * Check state of device with MACAddress.
	 * @return true if the device was detected or false
	 * @throws IOException when the detection fails for some reason
	 */
	virtual bool detect(const MACAddress &address) const = 0;

	/**
	 * Full scan of bluetooth network.
	 * This can find only visible devices.
	 * @return map of MAC addresses with names
	 */
	virtual std::map<MACAddress, std::string> scan() const = 0;

	/**
	 * Full scan of low energy bluetooth network.
	 * Sets parameters for low energy scan and open socket.
	 * @return list of MAC addresses with names
	 * @throws IOException when the detection fails for some reason
	 */
	virtual std::map<MACAddress, std::string> lescan(
			const Poco::Timespan &seconds) const = 0;

	/**
	 * Read information about the iterface.
	 */
	virtual HciInfo info() const = 0;
};

}
