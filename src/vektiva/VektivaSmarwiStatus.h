#pragma once

#include <Poco/Net/IPAddress.h>

namespace BeeeOn {

/**
 * @brief The class represents the status of the Smarwi device.
 * The status information is obtained by MQTT client in the status message.
 */
class VektivaSmarwiStatus {
public:
	VektivaSmarwiStatus(
		int status,
		int error,
		int ok,
		int ro,
		bool pos,
		int fix,
		Poco::Net::IPAddress ipAddress,
		int rssi);

	/**
	 * @brief Status is a number according to representing SmarWi state.
	 * There are several status codes such as:
	 * 200 - near frame opening
	 * 210 - opening
	 * 212 - closing but will open
	 * 220 - closing
	 * 230 - near frame closing
	 * 232 - closing from closed state, open a little bit
	 * 234 - closing from closed state, closing
	 * 250 - no action
	 * -1 - not calibrated not ready
	 * 130 - closing window, finishing calibration
	 * 10 - error
	 * 0 - Smarwi connected to the network
	 */
	int status() const;

	/**
	 * @brief In case any error occurs, it can be detected in this
	 * property. If correct state, 0 is sent or other number if any other
	 * error occurs. To be able to communicate with SmarWi when error occurs,
	 * "stop" command must be sent.
	 * Error codes:
	 * 0 - no error
	 * 10 - window seems locked
	 * 20 - movement timeout
	 */
	int error() const;

	/**
	 * @brief Shows whether Smarwi is in correct state.
	 * Values:
	 * 0 - error state
	 * 1 - available state
	 */
	int ok() const;

	/**
	 * @brief Signalizes whether the ridge is inside of the Smarwi or not.
	 * Values:
	 * 0 - ridge is outside of SmarWi
	 * 1 - ridge is inside of SmarWi
	 */
	int ro() const;

	/**
	 * @brief Shows in which position Smarwi is.
	 * Values:
	 * true - Smarwi is in open position
	 * false - Smarwi is in closed position
	 */
	bool pos() const;

	/**
	 * @brief Shows whether window is fixed by Smarwi or not. That means
	 * whether window can be moved easily without Smarwi trying to lock the
	 * window or Smarwi is holding the window.
	 * Values:
	 * 0 - unfixed
	 * 1 - fixed
	 */
	int fix() const;

	/**
	 * @brief Returns an IP address of the Smarwi.
	 */
	Poco::Net::IPAddress ipAddress() const;

	/**
	 * @brief Shows a current Wi-Fi signal strength.
	 */
	int rssi() const;
private:
	int m_status;
	int m_error;
	int m_ok;
	int m_ro;
	bool m_pos;
	int m_fix;
	Poco::Net::IPAddress m_ipAddress;
	int m_rssi;
};

}
