#pragma once

#include <functional>
#include <list>
#include <map>

#include <Poco/DigestEngine.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>

#include "exporters/QueuingStrategy.h"
#include "util/Journal.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief JournalQueuingStrategy implements persistent temporary storing of SensorData
 * into a filesystem structure. It controls contents of a selected directory. The contents
 * consists of buffer files and an index file (journal).
 *
 * The JournalQueuingStrategy maintains 3 kinds of files:
 *
 * - buffers - files named after their SHA-1 checksum (Git-like) containing serialized
 *   SensorData instances in a line-oriented way with CRC32 protection per-record
 *
 * - index - index of buffer files and byte offsets into them implemented as a journal
 *   (mostly append only file)
 *
 * - locks - when writing a file at once to disk (mostly buffers), a temporary lock
 *   files are created
 *
 * Writing data into the storage are controlled by the bytesLimit. If the limit is
 * reached by all persisted files (both active or dangling), the JournalQueuingStrategy
 * tries to garbage collect unused (dangling) files and if it does not succeed then
 * it drops also valid data that were not peeked yet.
 */
class JournalQueuingStrategy : public QueuingStrategy, protected Loggable {
public:
	JournalQueuingStrategy();

	/**
	 * @brief Set the root directory where to create or use a storage.
	 */
	void setRootDir(const std::string &path);

	/**
	 * @returns rootDir where the repository is located.
	 */
	Poco::Path rootDir() const;

	/**
	 * @brief Disable garbage-collection entirely. This is useful for debugging
	 * or in case of a serious bug in the GC code.
	 */
	void setDisableGC(bool disable);

	/**
	 * @brief Disable dropping of oldest data. This is useful for debugging or
	 * in case of a serious bug in the dropping code.
	 */
	void setNeverDropOldest(bool neverDrop);

	/**
	 * @brief Set top limit for data consumed by the strategy in the filesystem.
	 * This counts all data files (also dangling buffers, locks and index).
	 * When setting to a negative value, it is treated as unlimited.
	 */
	void setBytesLimit(int bytes);

	/**
	 * @returns true if an over limit is detected for the given amount of consumed
	 * space in bytes.
	 */
	bool overLimit(size_t bytes) const;

	/**
	 * @brief Configure behaviour of index loading. If the index contains broken
	 * entries, we can either fail quickly or ignore such errors and load as much
	 * as possible. Ignoring erros can lead to data loss in some cases but it is
	 * more probable that we would just peek & pop some already peeked data.
	 * The setting is applied only during JournalQueuingStrategy::setup().
	 */
	void setIgnoreIndexErrors(bool ignore);

	/**
	 * @brief Setup the storage for the JournalQueuingStrategy. It creates
	 * new index or loads the existing one. All buffers present in the index
	 * are checked and registered for future use.
	 */
	virtual void setup();

	/**
	 * @brief The call might call precacheEntries() to load up to 1 entry.
	 * @returns true if there is no buffer containing peekable data.
	 */
	bool empty() override;

	/**
	 * @brief Push the given data into a separate buffer and update
	 * the index accordingly. In case of serious failure, the method
	 * can throw exceptions.
	 */
	void push(const std::vector<SensorData> &data) override;

	/**
	 * @brief Peek the given count of data off the registered buffers
	 * starting from the oldest one. Calling this method is stable
	 * (returns the same results) until the pop() method is called.
	 * Index is not affected by this call.
	 *
	 * It is not expected to peek large amounts of data because the
	 * peek() call caches all such data in RAM.
	 */
	size_t peek(std::vector<SensorData> &data, size_t count) override;

	/**
	 * @brief Pop the count amount of data off the registered buffers.
	 * It updates index accordingly.
	 */
	void pop(size_t count) override;

protected:
	typedef std::function<void(
		const std::string &name,
		size_t offset,
		Poco::Timestamp &newest)> BrokenHandler;
	/**
	 * @brief Performs all the necessary steps done when calling setup().
	 * The given function is called when a broken buffer is detected.
	 * @returns timestamp of the newest detected buffered data.
	 */
	Poco::Timestamp initIndexAndScan(BrokenHandler broken);

	/**
	 * @brief Initialize the journaling index either by creating a new
	 * empty one or by loading the existing one.
	 */
	void initIndex(const Poco::Path &index);

	/**
	 * @brief Pre-scan all buffers in the index, check their consistency,
	 * collect some information (entries counts, errors, etc.) and update
	 * the index after any changes or fixes.
	 */
	void prescanBuffers(Poco::Timestamp &newest, BrokenHandler broken);

	/**
	 * @brief Report statistics about buffers.
	 */
	void reportStats(const Poco::Timestamp &newest) const;

	/**
	 * @brief Inspect buffer of the given name.
	 */
	void inspectAndRegisterBuffer(
		const std::string &name,
		size_t offset,
		Poco::Timestamp &newest);

	/**
	 * @returns the underlying index.
	 */
	Journal::Ptr index() const;

	/**
	 * @brief Write data safely to a file and return its name.
	 */
	std::string writeData(
		const std::string &data,
		bool force = true) const;

	/**
	 * @brief Remove the given file if possible. If usuallyFails is true,
	 * than the method does not log errors.
	 */
	bool whipeFile(Poco::File file, bool usuallyFails = false) const;

	/**
	 * @brief Collect all referenced buffers. Such buffers must not be
	 * deleted and should not be affected in anyway as they are suspects
	 * of peek()/pop() API.
	 */
	void collectReferenced(std::set<std::string> &referenced) const;

	/**
	 * @brief Perform garbage collection to ensure that at least
	 * the given bytes amount of space is available for writing.
	 */
	bool garbageCollect(const size_t bytes);

	/**
	 * @brief Drop oldest valid data to ensure that at least the
	 * given bytes amount of space is available for writing.
	 */
	void dropOldestBuffers(const size_t bytes);

	/**
	 * @returns amount of bytes occupied by active buffers and index.
	 */
	size_t bytesUsed() const;

	/**
	 * @returns amount of bytes occupied by all files related to the
	 * JournalQueuingStrategy (including lock files and dangling buffers).
	 */
	size_t bytesUsedAll() const;

	/**
	 * @returns path to the given name relative to the rootDir.
	 */
	Poco::Path pathTo(const std::string &name) const;

	/**
	 * @returns string representation of the given timestamp.
	 */
	static std::string tsString(const Poco::Timestamp &t);

	/**
	 * @brief An instance of Entry represents a single record
	 * in the FileBuffer. Such record contains a single SensorData
	 * instance. Moreover, name of the source buffer and offset
	 * after the parsed data is provided.
	 */
	class Entry {
	public:
		Entry(
			const SensorData &data,
			const std::string &buffer,
			const size_t nextOffset);

		const SensorData &data() const;
		std::string buffer() const;
		size_t nextOffset() const;

	private:
		SensorData m_data;
		std::string m_buffer;
		size_t m_nextOffset;
	};

	/**
	 * @brief The call ensures that there are up to count entries into
	 * the m_entryCache. If there is not enough data in the m_entryCache,
	 * more data is read from the appropriate buffer. This call helps to
	 * ensure that we do not read any data twice. Also, in case a buffer
	 * becomes unreadable for some reason, it would no be read again.
	 * The index is not affected by this call.
	 */
	size_t precacheEntries(size_t count);

	/**
	 * @brief Read up to count entries sequentially from buffers. For each
	 * entry, call the given method proc. Buffers' offsets are being updated
	 * during this process. The method itself does not modify the index in anyway.
	 */
	size_t readEntries(
		std::function<void(const Entry &entry)> proc,
		size_t count);

	/**
	 * @brief Helper struct with statistics collected during
	 * an inspection of a FileBuffer instance.
	 */
	struct FileBufferStat {
		Poco::Timestamp oldest = Poco::Timestamp::TIMEVAL_MAX;
		Poco::Timestamp newest = Poco::Timestamp::TIMEVAL_MIN;
		size_t bytes = 0;
		size_t offset = 0;
		size_t broken = 0;
		size_t count = 0;

		void update(const Poco::Timestamp &timestamp);
	};

	/**
	 * @brief Representation of a persistent file buffer that contains
	 * entries holding the stored SensorData.
	 */
	class FileBuffer {
	public:
		FileBuffer(
			const Poco::Path &path,
			size_t offset,
			size_t size);

		std::string name() const;
		Poco::Path path() const;
		size_t offset() const;
		size_t size() const;

		/**
		 * @returns true if the offset is greater than size and
		 * thus the buffer contains no more data to be scanned.
		 */
		bool exhausted() const;

		/**
		 * @brief Read all entries from the current offset.
		 * The offset is updated to point after the read data
		 * (even in case of an exception).
		 */
		size_t readEntries(
			std::function<void(const Entry &entry)> proc);

		/**
		 * @brief Read up to count entries from the current
		 * offset. The offset is updated to point after the
		 * read data (even in case of an exception).
		 */
		size_t readEntries(
			std::function<void(const Entry &entry)> proc,
			const size_t count);

		/**
		 * @brief
		 */
		void inspectAndVerify(
			const Poco::DigestEngine::Digest &digest,
			FileBufferStat &stat) const;

		/**
		 * @brief Format the given data into the form as expected
		 * by the readEntries() method.
		 */
		static std::string formatEntries(
			const std::vector<SensorData> &data);

	protected:
		/**
		 * @returns parsed data from the given line.
		 */
		SensorData parseData(const std::string &line) const;

		/**
		 * @brief Scan for up to count entries from the current
		 * offset. The parameter bytes is updated continuously
		 * while reading any bytes and it would up to date even
		 * in case of an exception.
		 */
		size_t scanEntries(
			size_t offset,
			std::function<void(const Entry &entry)> proc,
			size_t &bytes,
			const size_t count) const;

		/**
		 * @brief Scan for up to count entries from the given
		 * input stream. The parameter bytes is updated continuously
		 * while reading any bytes and it would up to date even
		 * in case of an exception.
		 */
		size_t scanEntries(
			std::istream &in,
			std::function<void(const Entry &entry)> proc,
			size_t &bytes,
			const size_t count) const;

	private:
		Poco::Path m_path;
		size_t m_offset;
		size_t m_size;
	};

	/**
	 * @brief Register the given buffer with the strategy.
	 * The index is not updated by this call.
	 */
	void registerBuffer(
		const FileBuffer &buffer,
		const FileBufferStat &stat);

private:
	Poco::Path m_rootDir;
	bool m_gcDisabled;
	bool m_neverDropOldest;
	ssize_t m_bytesLimit;
	bool m_ignoreIndexErrors;
	Journal::Ptr m_index;

	/**
	 * @brief Buffers known to be valid. The peek operation reads buffers
	 * from this list (the oldest buffers first).
	 */
	std::list<FileBuffer> m_buffers;

	/**
	 * @brief Buffers that have been exhausted and waiting to be popped.
	 * The state of those buffers are not yet reflected in the index so
	 * they must not be dropped.
	 */
	std::map<std::string, FileBuffer> m_exhausted;

	/**
	 * @brief Peeked entries waiting to be popped.
	 */
	std::list<Entry> m_entryCache;
};

}
