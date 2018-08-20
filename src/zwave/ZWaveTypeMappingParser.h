#pragma once

#include <utility>

#include "util/Loggable.h"
#include "util/XmlTypeMappingParser.h"

namespace BeeeOn {

typedef std::pair<uint8_t, uint8_t> ZWaveType;

/**
 * @brief ZWaveTypeMappingParser can parse XML files defining mappings
 * between Z-Wave command classes and BeeeOn ModuleTypes.
 */
class ZWaveTypeMappingParser : public XmlTypeMappingParser<ZWaveType> {
public:
	ZWaveTypeMappingParser();

protected:
	/**
	 * @brief Parse the given DOM node, extract attributes <code>command-class</code>
	 * and <code>index</code> and returned it as <code>ZWaveType(cc, index)</code>.
	 */
	ZWaveType parseTechType(const Poco::XML::Node &node) override;

	std::string techTypeRepr(const ZWaveType &type) override;
};

}
