#include <Poco/Checksum.h>
#include <Poco/File.h>
#include <Poco/NumberParser.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "util/DataReader.h"
#include "util/DataWriter.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const unsigned long DataReader::CHECKSUM_WIDTH = DataWriter::CHECKSUM_WIDTH;

DataReader::DataReader(istream &input):
	m_nextValid(false),
	m_input(input),
	m_dataRead(0)
{
}

bool DataReader::hasNext()
{
	if (m_nextValid)
		return true;

	return prefetchNext();
}

string DataReader::next()
{
	if (m_nextValid) {
		m_nextValid = false;
		++m_dataRead;
		return m_data;
	}

	if (prefetchNext()) {
		m_nextValid = false;
		++m_dataRead;
		return m_data;
	}

	throw IllegalStateException("no more data available");
}

uint32_t DataReader::checksum(string &data) const
{
	Checksum realChecksum;
	realChecksum.update(data);

	return realChecksum.checksum();
}

size_t DataReader::skip(size_t count)
{
	logger().information("attempting to skip %d valid data", count);

	size_t skipped = 0;

	while (count-- && prefetchNext())
		++skipped;

	m_nextValid = false;
	m_data.clear();

	logger().debug("skipped %d valid data", skipped);

	return skipped;
}

size_t DataReader::dataRead() const
{
	return m_dataRead;
}

bool DataReader::prefetchNext()
{
	m_data.clear();

	size_t skipped = 0;

	while (true) {
		try {
			string line;
			getline(m_input, line);

			if (m_input.eof()) {
				if (skipped > 0)
					logger().warning("EOF reached, skipped %d invalid data", skipped);

				return false;
			}

			uint32_t savedChecksum;

			savedChecksum = NumberParser::parseHex(line.substr(0, CHECKSUM_WIDTH));
			m_data = line.substr(DataWriter::CHECKSUM_WIDTH);

			if (checksum(m_data) == savedChecksum) {
				if (skipped > 0)
					logger().warning("skipped %d invalid data", skipped);

				m_nextValid = true;
				return true;
			}
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
		}
		catch (exception &e) {
			poco_critical(logger(), e.what());
		}
		catch (...) {
			poco_critical(logger(), "unknown error occurred while reading data");
		}

		++skipped;
	}
}
