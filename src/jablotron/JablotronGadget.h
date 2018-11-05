#pragma once

#include <list>
#include <string>
#include <vector>

#include <Poco/Timespan.h>

#include "model/ModuleType.h"
#include "model/RefreshTime.h"
#include "model/SensorValue.h"
#include "jablotron/JablotronReport.h"

namespace BeeeOn {

/**
 * @brief Representation of a real Jablotron Gadget registered
 * inside the associated Turris Dongle.
 */
class JablotronGadget {
public:
	enum Type {
		NONE = -1,
		AC88,
		JA80L,
		JA81M,
		JA82SH,
		JA83M,
		JA83P,
		JA85ST,
		RC86K,
		TP82N,
	};

	/**
	 * @brief Information about a Jablotron Gadget device type.
	 * Gadget types are distinguished by their address. A certain
	 * address range denotes a gadget device type.
	 */
	struct Info {
		const uint32_t firstAddress;
		const uint32_t lastAddress;
		const Type type;
		const RefreshTime refreshTime;
		const std::list<ModuleType> modules;

		operator bool() const;
		bool operator !() const;
		std::string name() const;

		/**
		 * @brief Parses the data payload of the given report and converts
		 * it into BeeeOn-specific data format.
		 */
		std::vector<SensorValue> parse(const JablotronReport &report) const;

		/**
		 * @returns gadget info instance based on the given address
		 */
		static Info resolve(const uint32_t address);

		/**
		 * @returns primary address of the gadget which is usually the
		 * address itself unless it is an address of RC-86K gadget
		 */
		static uint32_t primaryAddress(const uint32_t address);

		/**
		 * @returns secondary address of the gadget.
		 * @throws NotFoundException when the address is not of type RC-86K
		 */
		static uint32_t secondaryAddress(const uint32_t address);
	};

	JablotronGadget(
		const unsigned int slot,
		const uint32_t address,
		const Info &info);

	unsigned int slot() const;
	uint32_t address() const;
	const Info &info() const;

	/**
	 * @returns true if the gadget represents a secondary part which
	 * applies especially to RC-86K.
	 */
	bool isSecondary() const;

	std::string toString() const;

private:
	const unsigned int m_slot;
	const uint32_t m_address;
	const Info m_info;

	static const std::vector<Info> GADGETS;
};

}
