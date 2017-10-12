#pragma once

#include <istream>

#include "util/Loggable.h"
#include "util/DataIterator.h"

namespace BeeeOn {

/**
 * @class DataReader
 * @brief Serves to read and verify data written by the DataWriter.
 *
 * Invalid data (wrong format or wrong checksum) from the input stream are skipped.
 */
class DataReader : public DataIterator, protected Loggable {
public:
	const static unsigned long CHECKSUM_WIDTH;

	explicit DataReader(std::istream &input);

	/**
	 * @brief Serves to skip the given count of data from the input stream.
	 *
	 * If the given count is greater than or equal to the count of data in input stream, all data are skipped.
	 *
	 * @param count Count of data to be skipped.
	 * @return Count of skipped data.
	 */
	size_t skip(size_t count);

	/**
	 * @brief Informs if it is possible to get next valid data.
	 * @return True if the data are already loaded and not read yet. Otherwise tries to load next data, in the case of success,
	 * returns true, otherwise false.
	 */
	bool hasNext() override;

	/**
	 * @brief Serves to access next data from the input stream.
	 *
	 * However you can access the data through this method without calling the method hasNext(), it is recommended
	 * to ensure if there are next valid data, because if there are not, this method throws an exception.
	 *
	 * @return If there are already loaded data, returns them, else tries to load next data and in the case of success,
	 * returns them.
	 * @throw IllegalStateException in the case of no next valid data are available.
	 */
	std::string next() override;

	/**
	 * @return Count of data read from the input stream.
	 */
	size_t dataRead() const;

private:
	bool prefetchNext();
	uint32_t checksum(std::string &data) const;

private:
	bool m_nextValid;
	std::istream &m_input;
	size_t m_dataRead;
	std::string m_data;
};

}
