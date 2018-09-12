#pragma once

#include <list>
#include <set>

#include <Poco/Mutex.h>

#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "core/DeviceManager.h"
#include "hotplug/HotplugListener.h"
#include "jablotron/JablotronController.h"
#include "jablotron/JablotronGadget.h"
#include "jablotron/JablotronReport.h"
#include "model/ModuleType.h"
#include "util/BackOff.h"

namespace BeeeOn {

/**
 * @brief JablotronDeviceManager utilizes the JablotronController to communicate
 * with a Turris Dongle to receive reports (JablotronReport) and issue commands.
 * It maintains pairing of individual gadgets (JablotronGadget) and provides 3
 * controllable (virtual) devices: PGX, PGY and Siren that can be associated with
 * certain Jablotron devices (AC-88, JA-80L, etc.).
 *
 * Certain gadgets (RC-86K) can be dual - having 2 addresses. Such gadgets are
 * treated as a single device with primary and secondary address. The primary
 * address is used for device ID generation. Both addresses are registered,
 * checked and unregistered from slots.
 *
 * The Turris Dongle cannot discover devices. It only lists devices registered
 * with itself. Thus, to add a new device, it must be registered either physically
 * or by sending device-accept command with the appropriate device ID. Because
 * of this, the unpair operation does not unregister gadgets from slots as default
 * (this can be changed by property unpairErasesSlot).
 *
 * The PGX, PGY and Siren are paired differently. Everytime when the device-accept
 * is received from any of them, the enroll is send. If the target device is in
 * the appropriate learning mode, it would react on the control change requests.
 * Unpairing for PGX, PGY and Siren does nothing but affecting the BeeeOn device
 * pairing cache. The PGY is enrolled by sending 2 TX ENROLL:1 packets with an
 * appropriate gap. The gap is configurable (pgyEnrollGap) but it should be at
 * least few seconds to work properly.
 */
class JablotronDeviceManager : public DeviceManager, public HotplugListener {
public:
	JablotronDeviceManager();

	/**
	 * @brief The BeeeOn unpair operation marks a device as unpaired.
	 * It can also unregister the gadget from Turris Dongle. This however
	 * makes impossible to pair the gadget back again because the Turris
	 * Dongle has no way how to discover devices around. A physical
	 * registration must be done manually in that case.
	 */
	void setUnpairErasesSlot(bool erase);

	/**
	 * @brief Set BackOffFactory to be used while sending TX packets
	 * to set PGX, PGY or Siren. The backoff controls how many times
	 * the TX packet would be sent and how long delay to use between
	 * each two consecutive packets.
	 */
	void setTxBackOffFactory(BackOffFactory::Ptr factory);

	/**
	 * @brief Configure gap between two TX ENROLL packets while
	 * pairing the PGY. At least 3 seconds gap was experimentally
	 * shown to work.
	 */
	void setPGYEnrollGap(const Poco::Timespan &gap);

	/**
	 * @brief Erase all slots registered in the dongle when connected
	 * and probed successfully.
	 */
	void setEraseAllOnProbe(bool erase);

	/**
	 * @brief Register the given list of address when the dongle is
	 * connected and probed successfully (after the potential erase).
	 */
	void setRegisterOnProbe(const std::list<std::string> &addresses);

	/**
	 * @see JablotronController::setMaxProbeAttempts
	 */
	void setMaxProbeAttempts(int count);

	/**
	 * @see JablotronController::setProbeTimeout
	 */
	void setProbeTimeout(const Poco::Timespan &timeout);

	/**
	 * @see JablotronController::setIOReadTimeout
	 */
	void setIOJoinTimeout(const Poco::Timespan &timeout);

	/**
	 * @see JablotronController::setIOReadTimeout
	 */
	void setIOReadTimeout(const Poco::Timespan &timeout);

	/**
	 * @see JablotronController::setIOErrorSleep
	 */
	void setIOErrorSleep(const Poco::Timespan &delay);

	void onAdd(const HotplugEvent &e) override;
	void onRemove(const HotplugEvent &e) override;

	void run() override;
	void stop() override;

	/**
	 * @brief Receive list of paired devices and reflect this in the
	 * connected dongle if any.
	 */
	void handleRemoteStatus(
		const DevicePrefix &prefix,
		const std::set<DeviceID> &devices,
		const DeviceStatusHandler::DeviceValues &values) override;

protected:
	/**
	 * @brief Confirm pairing of the given device. If it is PGX, PGY
	 * or Siren, we send the TX ENROLL:1 packet(s). If pairing a gadget,
	 * we check the available slots register it unless it is already
	 * there. If the gadget has a secondary address, both of them
	 * are registered.
	 */
	void handleAccept(const DeviceAcceptCommand::Ptr cmd) override;

	/**
	 * @brief Discover all slots and report gadgets that are not paired.
	 * Also, report devices PGX, PGY and Siren.
	 */
	AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &timeout) override;

	/**
	 * @brief Unpair the given device. If it is PGX, PGY or Siren, then only
	 * the device cache is updated. When unpairing a gadget, it is unregistered
	 * from its slot if the unpairErasesSlot property is true.
	 * If the gadget has a secondary address, both of them are unregistered.
	 */
	AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
			const DeviceID &id,
			const Poco::Timespan &timeout) override;

	/**
	 * @brief Set value of PGX, PGY or Siren. It sends the TX packet with
	 * ENROLL:0 and the appropriate states. The TX packet is sent multiple
	 * times based on the configured txBackOffFactory.
	 */
	AsyncWork<double>::Ptr startSetValue(
			const DeviceID &id,
			const ModuleID &module,
			const double value,
			const Poco::Timespan &timeout) override;

	/**
	 * @brief Recognizes compatible dongle by testing HotplugEvent property
	 * as <code>tty.BEEEON_DONGLE == jablotron</code>.
	 */
	std::string hotplugMatch(const HotplugEvent &e);

	/**
	 * @brief Dispatch information about the new device.
	 */
	void newDevice(
		const DeviceID &id,
		const std::string &name,
		const std::list<ModuleType> &types,
		const Poco::Timespan &refreshTime);

	/**
	 * @brief Accept gadget device to be paired.
	 */
	void acceptGadget(const DeviceID &id);

	/**
	 * @brief Enroll PGX, PGY or Siren.
	 */
	void enrollTX();

	/**
	 * @brief Unregister gadget from tis slot.
	 */
	void registerGadget(
		std::set<unsigned int> &freeSlots,
		const uint32_t address,
		const Poco::Timespan &timeout);

	/**
	 * @brief Unregister gadget from tis slot.
	 */
	void unregisterGadget(const DeviceID &id, const Poco::Timespan &timeout);

	/**
	 * @brief Report the asynchronous message via DeviceManager::ship().
	 */
	void shipReport(const JablotronReport &report);

	/**
	 * @brief Read all slots from the controller and return registered
	 * gadgets. Certain gadgets might be unresolved (their info would be
	 * invalid).
	 */
	std::vector<JablotronGadget> readGadgets(const Poco::Timespan &timeout);

	/**
	 * @brief Scan all slots and detect all registered gadgets, free (empty) slots
	 * and slots having unknown (unsupported) gadgets.
	 */
	void scanSlots(
		std::set<uint32_t> &registered,
		std::set<unsigned int> &freeSlots,
		std::set<unsigned int> &unknownSlots);

	/**
	 * @brief Initialize the newly connected dongle. Apply the eraseAllOnProbe
	 * and registerOnProbe settings.
	 */
	void initDongle();

	/**
	 * @brief Scan slots and synchronize with pairing cache. The devices that
	 * are paired in device cache and missing in the dongle configuration are
	 * paired. If there is not enough space, the registered slots with non-paired
	 * devices are overwritten.
	 */
	void syncSlots();

	/**
	 * @breif Generate device ID based on the address. If the
	 * given address comes from a secondary gadget, it is first
	 * converted to primary address.
	 */
	static DeviceID buildID(uint32_t address);

	/**
	 * @returns gadget address from the given id
	 */
	static uint32_t extractAddress(const DeviceID &id);

	static DeviceID pgxID();
	static DeviceID pgyID();
	static DeviceID sirenID();

	/**
	 * @brief Sleep for at least the given delay and not less.
	 */
	static void sleep(const Poco::Timespan &delay);

private:
	bool m_unpairErasesSlot;
	BackOffFactory::Ptr m_txBackOffFactory;
	Poco::Timespan m_pgyEnrollGap;
	bool m_eraseAllOnProbe;
	std::list<uint32_t> m_registerOnProbe;
	JablotronController m_controller;
	bool m_pgx;
	bool m_pgy;
	bool m_alarm;
	JablotronController::Beep m_beep;
	Poco::FastMutex m_lock;
};

}
