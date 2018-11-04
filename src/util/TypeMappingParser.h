#pragma once

#include <iosfwd>
#include <vector>

#include "model/ModuleType.h"

namespace BeeeOn {

/**
 * @brief TypeMappingParser is an abstract class providing method
 * parse() that reads a given definition file (inpus stream). The file
 * contains mapping definitions between the target technology-specific
 * data types and the BeeeOn data types (ModuleType).
 */
template <typename TechType>
class TypeMappingParser {
public:
	/**
	 * @brief Single mapping between technology-specific type
	 * and the ModuleType.
	 */
	typedef std::pair<TechType, ModuleType> TypeMapping;

	/**
	 * @brief Sequence of particular mappings.
	 */
	typedef std::vector<TypeMapping> TypeMappingSequence;

	virtual ~TypeMappingParser();

	/**
	 * @brief Parse the given input stream and construct sequence
	 * of mappings between a technology-specific data type and
	 * the BeeeOn ModuleType. The sequence is returned in the same
	 * order as found in the input stream.
	 */
	virtual TypeMappingSequence parse(std::istream &in) = 0;
};

template <typename TechType>
TypeMappingParser<TechType>::~TypeMappingParser()
{
}

}
