#pragma once

#include <string>

namespace BeeeOn {

/**
 * @class DataIterator
 * @brief Iterator over data represented as byte sequences.
 *
 * Provides interface to iterate through some data while having access to the std::string form of each sequence.
 */
class DataIterator {
public:
	virtual bool hasNext() = 0;
	virtual std::string next() = 0;
};

}
