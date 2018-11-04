#pragma once

#include <string>
#include <Poco/Logger.h>

#include "core/Exporter.h"
#include "util/Loggable.h"

namespace BeeeOn {

class SensorDataFormatter;

class NamedPipeExporter :
	public Exporter,
	public Loggable {
public:
	NamedPipeExporter();
	~NamedPipeExporter();

	/**
	 * Export data to named pipe
	 */
	bool ship(const SensorData &data) override;

	/**
	 * Set file path of named pipe (mkfifo)
	 */
	void setFilePath(const std::string &path);

	/**
	 * Set formatter for type of output
	 * @param m_formatter
	 */
	void setFormatter(SensorDataFormatter * formatter);

private:
	/**
	 * Create pipe file (mkfifo)
	 * @return file descriptor to open mkfifo
	 * @throw ExporterInitException, when cannot open
	 */
	int openPipe();

	/**
	 * Write SensorData in string format to named pipe
	 * @return result of the write
	 * @throw ExporterShipException, when cannot write
	 */
	bool writeAndClose(int fd, const std::string &msg);

	std::string m_pipePath;
	SensorDataFormatter *m_formatter;
};

}
