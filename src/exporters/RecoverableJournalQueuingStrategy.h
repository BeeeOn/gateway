#pragma once

#include "exporters/JournalQueuingStrategy.h"

namespace BeeeOn {

/**
 * @brief RecoverableJournalQueuingStrategy works the same way as the
 * JournalQueuingStrategy but it extends its behaviour with recovering
 * features. The RecoverableJournalQueuingStrategy can recover partially
 * broken and non-commited buffers.
 *
 * A buffer is recovered when:
 *
 * - it was not commited, i.e. the data.tmp file exists, and it is non-empty
 * - a buffer is referenced from index and has an invalid digest
 * - an existing buffer contains timestamps newer than the newest timestamp
 *   in buffers referenced from index
 *
 * In this way, we should cover the following situations:
 *
 * - power supply failure while writing a buffer:
 *   non-committed buffer, committed buffer not recorded in index
 * - non-volatile media failure (written data becomes invalid)
 *
 * The recovery process DOES NOT work in-situ. Buffers being recovered are
 * first loaded into memory and such buffers are not deleted unless written
 * back successfully.
 */
class RecoverableJournalQueuingStrategy : public JournalQueuingStrategy {
public:
	RecoverableJournalQueuingStrategy();

	/**
	 * @brief Disable running recovery of the data.tmp file.
	 */
	void setDisableTmpDataRecovery(bool disable);

	/**
	 * @brief Disable running recovery of broken buffers referenced
	 * from index. Such buffers are ignored (and potentially garbage
	 * collected).
	 */
	void setDisableBrokenRecovery(bool disable);

	/**
	 * @brief Disable running recovery of lost buffers - ie. buffers
	 * that were not appended to index but contain recent data.
	 */
	void setDisableLostRecovery(bool disable);

	void setup() override;

protected:
	/**
	 * @brief Collect files looking like buffers that are not referenced
	 * and thus are potentially recoverable for some reason.
	 */
	void collectRecoverable(std::list<Poco::File> &files) const;

	/**
	 * @brief Read the given file and parse its contents like it is a
	 * buffer. In this way, we read as much of entries as possible
	 * while skipping errors.
	 *
	 * @returns number of errors occured while parsing
	 */
	size_t recoverEntries(
		Poco::File file,
		std::vector<SensorData> &data) const;

	/**
	 * @brief Recover contents of the given file into a new file that
	 * has a valid digest. Empty files are deleted. If the given file
	 * represents a valid buffer with a valid digest, it is left untouched.
	 */
	std::string recoverBrokenBuffer(Poco::File file) const;

	/**
	 * @brief Recover the given file as it should be a buffer. This method is
	 * useful to recover file that are not named accordingly - ie. we are not
	 * sure about their validity. It calls recoverBrokenBuffer().
	 */
	std::string recoverBuffer(Poco::File tmpFile) const;

	/**
	 * @brief Recover the data.tmp file if present and append it to the index.
	 */
	void recoverTmpData(Poco::Timestamp &newest);

	/**
	 * @breif Recover a broken buffer and register it. We update the newest
	 * timestamp accordingly as the recovered buffer is valid and we have to
	 * count on it.
	 */
	void recoverBroken(
		const std::string &broken,
		Poco::Timestamp &newest);

	/**
	 * @brief From a list of potentially recoverable buffers, recover those
	 * that has newer timestamps than the newest timestamp registered in the
	 * index. Also, timestamp of the last index file modification is used to
	 * speed up this process.
	 */
	void recoverLost(
		std::list<Poco::File> &recoverable,
		const Poco::Timestamp &indexModified,
		const Poco::Timestamp &newest);

private:
	bool m_disableTmpDataRecovery;
	bool m_disableBrokenRecovery;
	bool m_disableLostRecovery;
};

}
