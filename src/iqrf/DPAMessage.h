#pragma once

#include <string>
#include <vector>

#include <Poco/SharedPtr.h>
#include <Poco/JSON/Object.h>

namespace BeeeOn {

/**
 * @brief The class represents DPA message that can be sent/received
 * from IQRF network.
 *
 * Each message contains a header:
 *
 *  - NADR(2B) - network address
 *  - PNUM(1B)- peripherals number
 *  - CMD(1B) - command identification
 *  - HWPID(2B) - hw profile
 */
class DPAMessage {
public:
	typedef Poco::SharedPtr<DPAMessage> Ptr;
	typedef uint16_t NetworkAddress;

	static uint16_t COORDINATOR_NODE_ADDRESS;
	static uint16_t DEFAULT_HWPID;

	virtual ~DPAMessage();

	/**
	 * @brief Network device address.
	 *
	 * Value:
	 *
	 *  - 00               IQMESH Coordinator
	 *  - 01-EF            IQMESH Node address
	 *  - F0-FB            Reserved
	 *  - FC               Local (over interface) device
	 *  - FD               Reserved
	 *  - FE               IQMESH temporary address
	 *  - FF               IQMESH broadcast address
	 *  - 100-FFFF         Reserved
	 *
	 *  @see https://www.iqrf.org/DpaTechGuide/start.htm (2.5 Message parameters)
	 */
	void setNetworkAddress(NetworkAddress node);
	NetworkAddress networkAddress() const;

	/**
	 * @brief Peripheral number:
	 *
	 *  - (0x00 - 0x1F reserved for embedded peripherals)
	 *  - (0x40 - 0x7F reserved for IQRF standard peripherals)
	 *
	 * Value:
	 *
	 *  - 00               COORDINATOR
	 *  - 01               NODE
	 *  - 02               OS
	 *  - 03               EEPROM
	 *  - 04               EEEPROM
	 *  - 05               RAM
	 *  - 06               LEDR
	 *  - 07               LEDG
	 *  - 08               SPI
	 *  - 09               IO
	 *  - 0A               Thermometer
	 *  - 0B               PWM [*]
	 *  - 0C               UART
	 *  - 0D               FRC
	 *  - 0E-1F            Reserved
	 *  - 20-3E            User peripherals
	 *  - 3F               Not available
	 *  - 40-7F            Reserved
	 *  - 80-FF            Not available
	 *
	 * @see https://www.iqrf.org/DpaTechGuide/start.htm (2.5 Message parameters)
	 */
	void setPeripheralNumber(uint8_t pNumber);
	uint8_t peripheralNumber() const;

	/**
	 * @brief Command specifying an action to be taken. Actually allowed value
	 * range depends on the peripheral type. The most significant bit
	 * is reserved for indication of DPA response message.
	 *
	 * Value:
	 *
	 *  - 0-3E             command value
	 *  - 3F               not available
	 *  - 40-7F            command value
	 *  - 80-FF            not available
	 *
	 * @see https://www.iqrf.org/DpaTechGuide/start.htm (2.5 Message parameters)
	 */
	void setPeripheralCommand(uint8_t pCommand);
	uint8_t peripheralCommand() const;

	/**
	 * @brief HW profile ID uniquely specifies the functionality of the
	 * device, the user peripherals it implements, its behavior etc.
	 * The only device having the same HWPID as the DPA request will
	 * execute the request. When 0xFFFF is specified then the device
	 * with any HW profile ID will execute the request. Note - HWPID
	 * numbers used throughout this document are fictitious ones.
	 *
	 * Value:
	 *
	 *  - 0000             Default HW Profile
	 *  - 0001-xxxE        Certified HW Profiles
	 *  - xxxF             User HW Profiles
	 *  - FFFF             Reserved
	 *
	 * @see https://www.iqrf.org/DpaTechGuide/start.htm (2.5 Message parameters)
	 */
	void setHWPID(uint16_t hwPID);
	uint16_t HWPID() const;

	/**
	 * @brief Converts vector of values to dpa string. Dpa string contains
	 * hex values separated by dot.
	 *
	 * Example: 01.00.06.83.ff.ff
	 */
	virtual std::string toDPAString() const =0;

	/**
	 * @brief An array of bytes.
	 *
	 * @see https://www.iqrf.org/DpaTechGuide/start.htm (2.5 Message parameters)
	 */
	void setPeripheralData(const std::vector<uint8_t> &data);
	std::vector<uint8_t> peripheralData() const;

protected:
	/**
	 * @brief Creates message with DPA content that includes address
	 * of node, number of peripheral, command for peripheral and hw PID.
	 */
	DPAMessage(
		NetworkAddress node,
		uint8_t pNumber,
		uint8_t pCommand,
		uint16_t hwPID,
		const std::vector<uint8_t> &pData
	);

private:
	NetworkAddress m_nodeAddress;
	uint8_t m_peripheralNumber;
	uint8_t m_peripheralCommand;
	uint16_t m_hwPID;
	std::vector<uint8_t> m_peripheralData;
};

}
