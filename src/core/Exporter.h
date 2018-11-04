#pragma once

namespace BeeeOn {

class SensorData;

/**
 * Interface for Exporter
 * Exports data in best effort (or better)
 */
class Exporter {
public:
	Exporter();
	virtual ~Exporter();

	/**
	 * Ensures export data in best effort to target destination
	 * @return true when successfully shipped (this is kind of a guarantee that the caller doesn't have to care about the data anymore).
	 * @return false when the Exporter cannot ship data temporarily (e.g. full output buffers).
	 * @throws Poco::IOException when a serious issue caused the Exporter to deny its service.
	 */
	virtual bool ship(const SensorData &data) = 0;

};

}
