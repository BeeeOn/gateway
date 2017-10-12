#include <Poco/Checksum.h>
#include <Poco/NumberFormatter.h>

#include "util/DataWriter.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

DataWriter::DataWriter(ostream &output):
	m_output(output)
{
}

size_t DataWriter::write(DataIterator &iterator)
{
	size_t dataWritten = 0;

	while (iterator.hasNext()) {
		string data = iterator.next();

		string hexChecksum;
		NumberFormatter::appendHex(hexChecksum, checksum(data), CHECKSUM_WIDTH);

		m_output << hexChecksum << data << endl;

		++dataWritten;
	}

	return dataWritten;
}

uint32_t DataWriter::checksum(const string &data) const
{
	Checksum checksum;
	checksum.update(data);
	return checksum.checksum();
}
