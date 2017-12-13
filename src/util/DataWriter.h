#pragma once

#include <ostream>

#include "util/DataIterator.h"

namespace BeeeOn {

/**
 * @class DataWriter
 * @brief Serves to write data with their checksum to the output stream.
 */
class DataWriter {
public:
	const static int CHECKSUM_WIDTH = 8;

	explicit DataWriter(std::ostream &output);

	/**
	 * @brief Writes all the data provided by the given DataIterator to the output stream.
	 *
	 * Each data are written to the one line in the format:
	 * 	 "HEXACHECKSUM""DATA"
	 *
	 * 	 - "HEXACHECKSUM" is CRC-32 checksum of the following data in hexadecimal form
	 * 	 - "DATA" is the exact data given by the DataIterator
	 *
	 * @param iterator An iterator through the data to be written.
	 * @return The count of the written data.
	 */
	size_t write(DataIterator &iterator);

private:
	uint32_t checksum(const std::string &data) const;

public:
	std::ostream &m_output;
};

}
