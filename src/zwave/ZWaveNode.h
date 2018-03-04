#pragma once

#include <set>
#include <string>

#include <Poco/Timespan.h>

namespace BeeeOn {

/**
 * @brief ZWaveNode represents information from the Z-Wave network about
 * a particular node. Each Z-Wave node is identified by a home ID and
 * node ID. The node ID is a locally unique identifier. The home ID is
 * a globally unique (usually random generated) defined by the hardware
 * controller.
 *
 * It can be in one of two states:
 *
 * - not-queried - the mutable properties are probably invalid yet
 * - queried - the mutable probably are valid (the device is fully queried)
 */
class ZWaveNode {
public:
	/**
	 * @brief Identity of a Z-Wave node that can be used separately
	 * without any instance of the ZWaveNode class.
	 */
	struct Identity {
		const uint32_t home;
		const uint8_t node;

		Identity(const uint32_t home, const uint8_t node);

		Identity &operator =(const Identity &other);

		bool operator ==(const Identity &other) const;
		bool operator !=(const Identity &other) const;
		bool operator <(const Identity &other) const;

		std::string toString() const;
	};

	/**
	 * @brief Command class representation of a Z-Wave value.
	 * We support only a subset of command classes that are
	 * relevant for using with the BeeeOn system.
	 */
	class CommandClass {
	public:
		enum {
			BASIC         = 32,
			SWITCH_BINARY = 37,
			SENSOR_BINARY = 48,
			SENSOR_MULTILEVEL = 49,
			BATTERY       = 128,
			WAKE_UP       = 132,
			ALARM         = 113,
		};

		CommandClass(
			uint8_t id,
			uint8_t index,
			uint8_t instance,
			const std::string &name = "");

		/**
		 * @returns command class's ID (SWITCH_BINARY, BATTERY, ...)
		 */
		uint8_t id() const;

		/**
		 * @returns index of the specific value represented by the command class
		 */
		uint8_t index() const;

		/**
		 * @returns identifier for situations when certain command class is duplicated
		 */
		uint8_t instance() const;

		/**
		 * @returns command class's name, it can be empty if not initialized
		 */
		std::string name() const;

		std::string toString() const;

		bool operator <(const CommandClass &cc) const;

	private:
		uint8_t m_id;
		uint8_t m_index;
		uint8_t m_instance;
		std::string m_name;
	};

	/**
	 * @brief Value coming from the Z-Wave network. It holds some
	 * data (usually sensor data) and metadata to identify the
	 * value semantics.
	 */
	class Value {
	public:
		Value(const ZWaveNode &node,
		      const CommandClass &cc,
		      const std::string &value,
		      const std::string &unit = "");

		Value(const Identity &node,
			const CommandClass &cc,
			const std::string &value,
			const std::string &unit = "");

		/**
		 * @returns the associated node's identity
		 */
		const Identity &node() const;

		/**
		 * @returns command class that's value is represented
		 */
		const CommandClass &commandClass() const;

		/**
		 * @returns value in string format (raw)
		 */
		std::string value() const;

		/**
		 * @returns unit that the value is represented in
		 */
		std::string unit() const;

		/**
		 * Interpret the value as a boolean. If the value cannot be
		 * represented as boolean, an exception is thrown.
		 *
		 * @throw Poco::SyntaxException
		 */
		bool asBool() const;

		/**
		 * Interpret the value as an unsigned 32-bit number stored
		 * in the hexadecimal format. If the value cannot be parsed,
		 * it throws an exception.
		 */
		uint32_t asHex32() const;

		/**
		 * Interpret the value as a double (real) number.
		 * If the value cannot be parsed, it throws an exception.
		 */
		double asDouble() const;

		/**
		 * Interpret the value as signed int. If the underlying value
		 * is real (double) and the argument floor is false then an
		 * exception is thrown.
		 *
		 * @param floor if the underlying cannot be interpreted as int,
		 * and floor is true, it would be interpreted as double and floored
		 */
		int asInt(bool floor = false) const;

		/**
		 * Interpret the underlying value as temperature. The supported
		 * units are C and F (according to Z-Wave). If the value is represented
		 * in F (Farenheit) a conversion is applied.
		 */
		double asCelsius() const;

		/**
		 * Interpret the underlying value as a value of luminance. The luminance
		 * is returned in lux. If the underlying value is represented in percent,
		 * a conversion is applied.
		 */
		double asLuminance() const;

		/**
		 * Interpret the underlying value as a value of PM 2.5. The expected and
		 * only supported unit is ug/m3.
		 */
		double asPM25() const;

		/**
		 * Interpret the underlying value as a value of time. The expected and
		 * only supported unit is seconds.
		 */
		Poco::Timespan asTime() const;

		std::string toString() const;

	private:
		Identity m_node;
		CommandClass m_commandClass;
		std::string m_value;
		std::string m_unit;
	};

	/**
	 * @brief Feature flags denoting supported features of a Z-Wave node.
	 */
	enum Support {
		SUPPORT_LISTENING = 0x01,
		SUPPORT_BEAMING   = 0x02,
		SUPPORT_ROUTING   = 0x04,
		SUPPORT_SECURITY  = 0x08,
		SUPPORT_ZWAVEPLUS = 0x10,
	};

	ZWaveNode(const Identity &id, bool controller = false);

	/**
	 * @returns home ID, it is always valid
	 */
	uint32_t home() const;

	/**
	 * @returns node ID, it is always valid
	 */
	uint8_t node() const;

	/**
	 * @returns node identity, it is always valid
	 */
	const Identity &id() const;

	/**
	 * @returns true if this node is the controller of the network,
	 * it is always valid
	 */
	bool controller() const;

	/**
	 * Set bitmap of support flags.
	 */
	void setSupport(uint32_t support);

	/**
	 * @returns bitmap of support flags
	 */
	uint32_t support() const;

	/**
	 * Set product ID reported by the node.
	 */
	void setProductId(uint16_t id);

	/**
	 * @returns product ID as reported by the node itself.
	 */
	uint16_t productId() const;

	/**
	 * Set name of the node product.
	 */
	void setProduct(const std::string &name);

	/**
	 * @returns name of the node product.
	 */
	std::string product() const;

	/**
	 * Set product type as reported by the node itself.
	 */
	void setProductType(uint16_t type);

	/**
	 * @returns product type as reported by the node itself.
	 */
	uint16_t productType() const;

	/**
	 * Set vendor ID as reported by the node itself.
	 */
	void setVendorId(uint16_t id);

	/**
	 * @returns vendor ID as reported by the node itself.
	 */
	uint16_t vendorId() const;

	/**
	 * Set name of the node's vendor (manufacturer).
	 */
	void setVendor(const std::string &vendor);

	/**
	 * @returns name of the node's vendor (manufacturer)
	 */
	std::string vendor() const;

	/**
	 * Set whether this node is already queried and thus
	 * the mutable properties are set.
	 */
	void setQueried(bool queried);

	/**
	 * @returns true if this node has been fully queried
	 */
	bool queried() const;

	void add(const CommandClass &cc);
	const std::set<CommandClass> &commandClasses() const;

	std::string toString() const;
	std::string toInfoString() const;

	bool operator <(const ZWaveNode &node) const;

private:
	Identity m_id;
	bool m_controller;
	bool m_queried;
	uint32_t m_support;
	uint16_t m_productId;
	std::string m_product;
	uint16_t m_vendorId;
	std::string m_vendor;
	uint16_t m_productType;
	std::set<CommandClass> m_commandClasses;
};

}
