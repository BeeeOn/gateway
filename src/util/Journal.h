#pragma once

#include <list>
#include <set>
#include <string>

#include <Poco/File.h>
#include <Poco/Mutex.h>
#include <Poco/Nullable.h>
#include <Poco/Path.h>
#include <Poco/SharedPtr.h>

#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Journal implements a simple journaling principle in filesystem.
 * A Journal instance represents an append-only persistent list of records.
 * Appending is an atomic operation. We either append the whole record or
 * append nothing.
 *
 * It is considered that the number of records is quite low (up to few kB).
 * Each record consists of a key and value. The key is an identifier of
 * some entity being changed. The value represents the change of its entity
 * to be recorded. Appending a new value for an existing key means that the
 * associated entity has been updated.
 *
 * A journal is a sequential structure which gives it several properties:
 * - efficient writes to a persistent storage (reduced seeking and erasing)
 * - appending data cannot destroy the previous contents
 * - reading from begining leads to a stable interpretation of its records
 *
 * Journal provides a way how to recover after a system or power failure.
 * Based on the journal contents, it is possible to reconstruct the most
 * recent consistent state.
 *
 * Each persisted record is protected by the CRC32 checksum to detect
 * incomplete writes or broken underlying storage. Records with invalid
 * checksum can be in some scenarios just skipped (leads to a data loss)
 * in other scenarios we might need to recover in a more complex ways
 * which are currently unsupported by the Journal class.
 *
 * To avoid infinite grow of the journal, it can be internally deduplicated
 * and thus rotated. After some time, several entities in the journal would
 * be contained multiple times with different values. However, only the most
 * recent value is valid for each entity. Those duplications can be avoid
 * once upon a time or when consuming too much of storage.
 *
 * There are two parameters that triggers rotations:
 * - duplicates factor - average amount of key duplicates
 * - minimal rewrite size - minimal size of the journal in bytes
 *
 * If there more duplicates than the valud of duplicates factor and the journal
 * is currently bigger then the minimal rewrite size, the journal automatically
 * deduplicates itself (during the flush operation) and writes its shrinked
 * version safely into the storage. The rewrite utilizes the SafeWriter to stay
 * as safe as possible and prevent any or most data loss possibilities.
 */
class Journal : protected virtual Loggable {
public:
	typedef Poco::SharedPtr<Journal> Ptr;

	/**
	 * @brief A single record in journal.
	 */
	struct Record {
		const std::string key;
		const std::string value;

		Record &operator =(const Record &other)
		{
			const_cast<std::string&>(this->key) = other.key;
			const_cast<std::string&>(this->value) = other.value;
			return *this;
		}

		bool operator ==(const Record &other) const
		{
			return key == other.key && value == other.value;
		}
	};

	Journal(
		const Poco::Path &file,
		double duplicatesFactor = 3,
		size_t minimalRewritesSize = 4096);
	virtual ~Journal();

	/**
	 * @brief Create empty journal if it does not exists yet.
	 * @returns true if created, false if it already exists
	 */
	bool createEmpty();

	/**
	 * @brief Check for common situations that might be a symptom
	 * of an invalid setup. We check whether the underlying file
	 * is readable, (optionally) writable and (optionally) whether
	 * it is a regular file. If the file does not exists, we test the
	 * parent directory whether it is readable and (optionally)
	 * wriable.
	 *
	 * @throws Poco::FileAccessDeniedException - file is not readable
	 * @throws Poco::FileReadOnlyException - file is not writable
	 * @throws Poco::InvalidArgumentException - file is not a regular file
	 */
	void checkExisting(bool regularFile = true, bool writable = true) const;

	/**
	 * @brief Load the journal from the underlying file. If the parameter
	 * recover is true, the method skips malformed entries (with invalid checkums).
	 * Otherwise, the method throws an exception.
	 */
	void load(bool recover = false);

	/**
	 * @brief Load the journal from the given stream. If the parameter
	 * recover is true, the method skips malformed entries (with invalid checkums).
	 * Otherwise, the method throws an exception.
	 *
	 * CAUTION: This operation may lead to have an inconsistent
	 * journal state between RAM and the underlying file when
	 * loading from another (underlated) data source.
	 */
	void load(std::istream &in, bool recover = false);

	/**
	 * @brief Check that the RAM journal representation is equivalent to the
	 * persistent representation in the underlying file.
	 */
	void checkConsistent() const;

	/**
	 * @brief Check that the RAM journal representation is equivalent to the
	 * representation readed from the given input stream.
	 */
	void checkConsistent(std::istream &in) const;

	/**
	 * @brief Append the key-value pair into the journal.
	 * @see Journal::append(const Record &, bool)
	 */
	void append(
		const std::string &key,
		const std::string &value,
		bool flush = true)
	{
		append({key, value}, flush);
	}

	/**
	 * @brief Append the given record into the journal. If the parameter flush
	 * is true (default), the record is immediatelly persisted and flushed.
	 * Otherwise, it is written into a waiting list to be flush in a batch.
	 */
	void append(const Record &record, bool flush = true);

	/**
	 * @brief Mark the given key as dropped.
	 * @see Journal::drop(const std::set<std::string> &, bool)
	 */
	void drop(const std::string &key, bool flush = true)
	{
		drop(std::set<std::string>{key}, flush);
	}

	/**
	 * @brief Mark all the given keys as dropped. The operation itself can be
	 * delayed when the parameter flush is false. Otherwise, all the keys are
	 * immediatelly persisted and flushed. Note that dropping multiple keys is
	 * NOT an atomic operation. We can end up with only few keys dropped in case
	 * of a serious failure.
	 */
	void drop(const std::set<std::string> &keys, bool flush = true);

	/**
	 * @brief Flush all records in the waiting list. If the current duplicates
	 * factor is high enough and the size of the journal is bigger than the
	 * minimal rewrite size, the deduplication is performed. In case of an I/O
	 * failure while deduplicating, it fallbacks to simple append with flush.
	 */
	void flush();

	/**
	 * @returns current state of each record, only the most recent records per key
	 * are returned this way (thus we have no access to the history by this method).
	 */
	std::list<Record> records() const;

	/**
	 * @returns value of record by the given key or null is it is not in the journal
	 */
	Poco::Nullable<std::string> operator [](const std::string &key) const;

	/**
	 * @breif Computes the current duplicates factor of the journal main records
	 * (waiting records are not counted).
	 */
	double currentDuplicatesFactor() const;

protected:
	void parseStream(std::istream &in, std::list<Record> &records) const;
	void parseStreamRecover(std::istream &in, std::list<Record> &records) const;
	void appendDrop(const std::string &key, bool flush);
	void dropInPlace(std::list<Record> &records, const std::string &key) const;

	double duplicatesFactor(const std::list<Record> &records) const;
	bool overMinimalSize() const;
	std::list<Record> recordsRaw() const;
	void interpret(std::list<Record> &records) const;
	void interpretAndFlush();
	void appendFlush();
	void rewriteAndFlush(const std::list<Record> &records);

	std::list<Record> &committed();
	std::list<Record> &dirty();

	void handleFailure(std::ostream &o) const;

	void checkRecord(const Record &record) const;
	std::string format(const Record &record, bool zeroSum = false) const;
	Record parse(const std::string &line, size_t lineno) const;
	size_t bytes(const std::list<Record> &records) const;

private:
	mutable Poco::Mutex m_lock;
	const Poco::File m_file;
	double m_duplicatesFactor;
	size_t m_minimalRewriteSize;
	std::list<Record> m_records;
	std::list<Record> m_dirty;
};

}
