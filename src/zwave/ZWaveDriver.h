#pragma once

#include <string>

#include <openzwave/Driver.h>

namespace BeeeOn {

class ZWaveDriver {
public:
	ZWaveDriver(const std::string &driverPath = "");

	/**
	 * Path to Z-Wave driver.
	 * @param driverPath
	 */
	void setDriverPath(const std::string &driverPath);

	/**
	 * Get driver path
	 * @return driver path
	 */
	std::string driverPath();

	/**
	 * Address a new driver for a Z-Wave controller.
	 * @return True if driver successfully added
	 */
	bool registerItself();

	/**
	 * Removes the driver for a Z-Wave controller.
	 * and closes the controller.
	 * @return True if the driver was removed, false if it could not be found
	 */
	bool unregisterItself();

private:
	/**
	 * Method detects type of plugged driver.
	 * Driver::ControllerInterface_Hid is device which contains "usb"
	 * string in path, other is Driver::ControllerInterface_Serial.
	 * Example of driver:
	 *  Driver::ControllerInterface_Hid - /dev/cu.usbserial
	 *  Driver::ControllerInterface_Serial - /dev/ttyACM0
	 */
	OpenZWave::Driver::ControllerInterface detectInterface() const;

private:
	std::string m_driver;
};

}
