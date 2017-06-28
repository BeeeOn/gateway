#pragma once

#include <map>
#include <set>

#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Util/Timer.h>

#include "commands/DeviceSetValueCommand.h"
#include "commands/GatewayListenCommand.h"
#include "core/DongleDeviceManager.h"
#include "io/SerialPort.h"
#include "jablotron/JablotronDevice.h"
#include "model/DeviceID.h"
#include "util/LambdaTimerTask.h"

namespace BeeeOn {

/**
 * JablotronDeviceManager manages a cache of devices associated with the
 * Jablotron dongle and paired on the server. Devices associated with the
 * dongle are not necessarily those paired by the user. Thus, the
 * associated devices are managed in the form <DeviceID, NULL>.
 *
 * When a device is paired by its user, the JablotronDeviceManager creates
 * a new instance of a JablotronDevice and manages such device as
 * <DeviceID, SharedPtr<JablotronDevice>>.
 *
 * Physical Jablotron devices can be associated with the dongle only
 * in a software-based way. Thus, they are associated by an instance
 * of the JablotronDeviceManager (and we know about it) or the dongle
 * must be physically connected to another device. Every time a dongle
 * is physical reconnected, the JablotronDeviceManager checks the
 * associated devices and the coherency of the device list thus maintained.
 *
 * However, if another process (by mistake or even intentionally) performs
 * a device reassociation with the dongle directly via the serial port
 * in parallel while the JablotronDeviceManaher is running, then the
 * JablotronDeviceManager could behave in an unexpected way. Such
 * behaviour is undefined.
 */
class JablotronDeviceManager : public DongleDeviceManager {
public:
	JablotronDeviceManager();

	bool accept(const Command::Ptr cmd) override;
	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

	std::string dongleMatch(const UDevEvent &e) override;
	void dongleAvailable() override;

	void stop() override;

private:
	enum MessageType {
		OK,
		ERROR,
		DATA,
		NONE,
	};

	/**
	 * Load device list from server and create JablotronDevice.
	 */
	void loadDeviceList();

	/**
	 * It shows Jablotron Turris Dongle firmware version.
	 * Example: TURRIS DONGLE V1.4
	 */
	void dongleVersion();
	bool wasTurrisDongleVersion(const std::string &message) const;

	void initJablotronDongle();
	void jablotronProcess();

	/**
	 * It picks next message from buffer and removes picked message from it.
	 * If buffer is empty, method reads data from serial into the buffer.
	 */
	MessageType nextMessage(std::string &message);
	MessageType messageType(const std::string &data);

	/**
	 * Extract serial number from string.
	 * Example of string: SLOT:01 [09491911]
	 */
	uint32_t extractSerialNumber(const std::string &message);

	/**
	 * Load devices from USB Dongle.
	 */
	void setupDongleDevices();

	void shipMessage(const std::string &message, JablotronDevice::Ptr jablotronDevice);

	/**
	 * It sends message to Jablotron dongle and checks if sent
	 * message is accepted.
	 *
	 * @return True if Turris dongle accepts message.
	 */
	bool transmitMessage(const std::string &msg, bool autoResult);

	void doSetValue(DeviceSetValueCommand::Ptr cmd, Answer::Ptr answer);
	void doListenCommand(
		const GatewayListenCommand::Ptr cmd, const Answer::Ptr answer);
	void doNewDevice(const DeviceID &deviceID,
		std::map<DeviceID, JablotronDevice::Ptr>::iterator &it);

	bool getResponse();
	bool isResponse(MessageType type);

private:
	SerialPort m_serial;
	Poco::Mutex m_lock;
	std::map<DeviceID, JablotronDevice::Ptr> m_devices;
	std::string m_buffer;

	MessageType m_lastResponse;
	Poco::Event m_responseRcv;
	Poco::AtomicCounter m_isListen;
	Poco::Util::Timer m_listenTimer;
};

}
