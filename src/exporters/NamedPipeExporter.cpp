#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Poco/Exception.h>

#include "di/Injectable.h"
#include "exporters/NamedPipeExporter.h"
#include "util/NullSensorDataFormatter.h"
#include "util/SensorDataFormatter.h"

#define ATTEMPTS_CREATE_PIPE 3

BEEEON_OBJECT_BEGIN(BeeeOn, NamedPipeExporter)
BEEEON_OBJECT_CASTABLE(Exporter)
BEEEON_OBJECT_TEXT("filePath", &NamedPipeExporter::setFilePath)
BEEEON_OBJECT_REF("formatter", &NamedPipeExporter::setFormatter)
BEEEON_OBJECT_END(BeeeOn, NamedPipeExporter)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

NamedPipeExporter::NamedPipeExporter() :
	m_formatter(&NullSensorDataFormatter::instance())
{
}

NamedPipeExporter::~NamedPipeExporter()
{
	if (!m_pipePath.empty())
		remove(m_pipePath.c_str());
}

bool NamedPipeExporter::ship(const SensorData &data)
{
	int fd = openPipe();

	if (fd < 0 && errno == ENXIO)
		return true;
	if (fd < 0 && errno == EINTR)
		return false;

	poco_assert(fd >= 0);

	try {
		return writeAndClose(fd, m_formatter->format(data) + "\n");
	}
	catch (...) {
		close(fd);
		throw;
	}
}

void NamedPipeExporter::setFilePath(const string &path)
{
	m_pipePath = path;
}

void NamedPipeExporter::setFormatter(SensorDataFormatter *formatter)
{
	m_formatter = formatter;
}

int NamedPipeExporter::openPipe()
{
	unsigned int attempts = ATTEMPTS_CREATE_PIPE;
	int fd;

	do { // forbid symlinks to be followed
		fd = open(m_pipePath.c_str(), O_WRONLY | O_NONBLOCK | O_NOFOLLOW);
		if (fd < 0 && errno == ENOENT) {
			mode_t permit = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

			if (mkfifo(m_pipePath.c_str(), permit) < 0) {
					throw IOException(
						"failed to initialize mkfifo "
						+ m_pipePath + ": "
						+ strerror(errno));
			}
			continue;
		}
		else if (fd < 0 && errno == ENXIO) {
			// when there is no reader, drop data
			return fd;
		}
		else if (fd < 0 && errno == EINTR) {
			// interrupt from user
			logger().notice("interrupt from user during init",
				__FILE__, __LINE__);
			return fd;
		}
		else if (fd < 0) {
			// others unexpected failures
			throw IOException(
				"failed to initialize named pipe "
				+ m_pipePath + ": "
				+ strerror(errno));
		}
	} while (fd < 0 && attempts-- > 0);

	if (fd < 0 && attempts == 0) {
		throw IOException(
			"failed to create and open mkfifo "
			+ m_pipePath + ": "
			+ strerror(errno));
	}

	struct stat st;
	if (fstat(fd, &st) < 0) {
		// should never happen
		close(fd);
		throw IOException(
			"failed to stat mkfifo "
			+ m_pipePath + ": "
			+ strerror(errno));
	}

	if (!S_ISFIFO(st.st_mode)) {
		// we opened non-fifo file, a race condition?
		close(fd);
		throw IOException(
			"file '" + m_pipePath
			+ "'' is not a fifo: "
			+ strerror(errno));
	}
	return fd;
}

bool NamedPipeExporter::writeAndClose(int fd, const string &msg)
{
	ssize_t totalLength = msg.size();
	ssize_t restLength = 0;

	do { // we are blocking here
		ssize_t writtenLength = write(fd, msg.c_str() + restLength, totalLength - restLength);

		if (writtenLength < 0 && errno == EINTR) {
			// interrupt from user
			logger().notice("interrupt from user during write",
				__FILE__, __LINE__);
			break;
		}
		if (writtenLength < 0) {
			close(fd);
			throw IOException(
				"failed to write fifo: "
				+ string(strerror(errno)));
		}

		restLength += writtenLength;
	} while(restLength < totalLength);

	if (logger().debug()) {
		logger().debug(
			"written " + to_string(restLength) + "/"
			+ to_string(totalLength) + " bytes",
			__FILE__, __LINE__);
	}

	close(fd);
	return restLength >= totalLength;
}
