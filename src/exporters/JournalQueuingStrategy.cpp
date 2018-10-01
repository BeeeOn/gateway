#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DigestStream.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>
#include <Poco/FileStream.h>
#include <Poco/Logger.h>
#include <Poco/NullStream.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>
#include <Poco/StreamCopier.h>
#include <Poco/String.h>
#include <Poco/SHA1Engine.h>

#include "di/Injectable.h"
#include "exporters/JournalQueuingStrategy.h"
#include "io/SafeWriter.h"
#include "util/ChecksumSensorDataFormatter.h"
#include "util/ChecksumSensorDataParser.h"
#include "util/JSONSensorDataFormatter.h"
#include "util/JSONSensorDataParser.h"

BEEEON_OBJECT_BEGIN(BeeeOn, JournalQueuingStrategy)
BEEEON_OBJECT_CASTABLE(QueuingStrategy)
BEEEON_OBJECT_PROPERTY("rootDir", &JournalQueuingStrategy::setRootDir)
BEEEON_OBJECT_PROPERTY("disableGC", &JournalQueuingStrategy::setDisableGC)
BEEEON_OBJECT_PROPERTY("neverDropOldest", &JournalQueuingStrategy::setNeverDropOldest)
BEEEON_OBJECT_PROPERTY("bytesLimit", &JournalQueuingStrategy::setBytesLimit)
BEEEON_OBJECT_PROPERTY("ignoreIndexErrors", &JournalQueuingStrategy::setIgnoreIndexErrors)
BEEEON_OBJECT_HOOK("done", &JournalQueuingStrategy::setup)
BEEEON_OBJECT_END(BeeeOn, JournalQueuingStrategy)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

static const RegularExpression BUFFER_REGEX("^[a-fA-F0-9]{40}$");
static const RegularExpression INDEX_REGEX("^index$");
static const RegularExpression INDEX_LOCK_REGEX("^index.lock$");

JournalQueuingStrategy::JournalQueuingStrategy():
	m_gcDisabled(false),
	m_neverDropOldest(false),
	m_bytesLimit(-1),
	m_ignoreIndexErrors(true)
{
}

void JournalQueuingStrategy::setRootDir(const string &path)
{
	m_rootDir = path;
}

Path JournalQueuingStrategy::rootDir() const
{
	return m_rootDir;
}

void JournalQueuingStrategy::setDisableGC(bool disable)
{
	m_gcDisabled = disable;
}

void JournalQueuingStrategy::setNeverDropOldest(bool neverDrop)
{
	m_neverDropOldest = neverDrop;
}

void JournalQueuingStrategy::setBytesLimit(int bytes)
{
	m_bytesLimit = bytes < 0 ? -1 : bytes;
}

bool JournalQueuingStrategy::overLimit(size_t bytes) const
{
	if (m_bytesLimit < 0)
		return false;

	const size_t limit = m_bytesLimit;
	return bytes >= limit;
}

void JournalQueuingStrategy::setIgnoreIndexErrors(bool ignore)
{
	m_ignoreIndexErrors = ignore;
}

void JournalQueuingStrategy::initIndex(const Path &index)
{
	m_index = new Journal(index);

	if (!m_index->createEmpty()) {
		logger().notice(
			"loading index from " + index.toString(),
			__FILE__, __LINE__);

		m_index->checkExisting(false, true);
		m_index->load(m_ignoreIndexErrors);
	}
	else {
		logger().notice(
			"empty index created at " + index.toString(),
			__FILE__, __LINE__);
	}
}

void JournalQueuingStrategy::inspectAndRegisterBuffer(
		const string &name,
		size_t offset,
		Timestamp &newest)
{
	File file = pathTo(name);
	size_t size = 0;

	try {
		size = file.getSize();
	}
	BEEEON_CATCH_CHAIN_ACTION(logger(),
		m_index->drop(name, false);
		return) // non-recoverable, just skip it

	FileBuffer buffer(file.path(), offset, size);
	FileBufferStat stat;

	if (logger().debug()) {
		logger().debug(
			"inspecting buffer " + buffer.name(),
			__FILE__, __LINE__);
	}

	buffer.inspectAndVerify(
		DigestEngine::digestFromHex(name),
		stat);

	registerBuffer(buffer, stat);
	newest = max(newest, stat.newest);
}

void JournalQueuingStrategy::prescanBuffers(Timestamp &newest, BrokenHandler broken)
{
	for (const auto &record : m_index->records()) {
		const auto &name = record.key;

		if (logger().debug()) {
			logger().debug(
				"scanning buffer " + name
				+ " with offset " + record.value,
				__FILE__, __LINE__);
		}

		if (!BUFFER_REGEX.match(name)) {
			logger().warning(
				"unexpected name of buffer: " + name,
				__FILE__, __LINE__);

			m_index->drop(name, false);
			continue;
		}

		uint32_t offset = 0;
		if (!NumberParser::tryParseHex(record.value, offset)) {
			logger().error(
				"failed to parse offset of buffer " + name,
				__FILE__, __LINE__);

			m_index->drop(name, false);
			continue;
		}

		try {
			inspectAndRegisterBuffer(name, offset, newest);
		}
		BEEEON_CATCH_CHAIN_ACTION(logger(),
			broken(name, offset, newest));
	}

	m_index->flush();
}

void JournalQueuingStrategy::setup()
{
	const auto &newest = initIndexAndScan(
		[&](const string &name, size_t, Timestamp &)
		{
			index()->drop(name, false);
			whipeFile(pathTo(name));	
		});

	reportStats(newest);
}

Timestamp JournalQueuingStrategy::initIndexAndScan(BrokenHandler broken)
{
	m_buffers.clear();
	m_exhausted.clear();
	m_entryCache.clear();

	File rootDir(m_rootDir);
	rootDir.createDirectories();

	const auto &index = pathTo("index");
	initIndex(index);

	Timestamp newest = Timestamp::TIMEVAL_MIN;
	prescanBuffers(newest, broken);

	return newest;
}

void JournalQueuingStrategy::reportStats(const Timestamp &newest) const
{
	logger().information(
		"used " + to_string(bytesUsed())
		+ " B, total " + to_string(bytesUsedAll())
		+ " B, newest timestamp: " + tsString(newest),
		__FILE__, __LINE__);
}

bool JournalQueuingStrategy::whipeFile(File file, bool usuallyFails) const
{
	if (logger().debug()) {
		logger().debug(
			"removing file " + file.path() + (usuallyFails? " (muted)" : ""),
			__FILE__, __LINE__);
	}

	if (usuallyFails) {
		try {
			file.remove(true);
			return true;
		}
		catch (...) {}
	}
	else {
		try {
			file.remove(true);
			return true;
		}
		BEEEON_CATCH_CHAIN(logger())
	}

	return false;
}

void JournalQueuingStrategy::registerBuffer(
		const FileBuffer &buffer,
		const FileBufferStat &stat)
{
	for (const auto &one : m_buffers) {
		if (one.name() == buffer.name()) {
			logger().debug(
				"ignoring duplicate registration of buffer "
				+ buffer.name(),
				__FILE__, __LINE__);
			return;
		}
	}

	logger().information(
		"registering buffer " + buffer.name() + " ("
		+ to_string(buffer.offset()) + "): "
		+ to_string(stat.bytes) + "/"
		+ to_string(stat.count) + "/"
		+ to_string(stat.broken) + " with period "
		+ tsString(stat.oldest) + ".." + tsString(stat.newest),
		__FILE__, __LINE__);

	m_buffers.emplace_back(buffer);
}

string JournalQueuingStrategy::writeData(
		const string &data,
		bool force) const
{
	SafeWriter writer(pathTo("data.tmp"));
	writer.stream(force) << data;

	const auto &state = writer.finalize();

	if (state.second != data.size()) {
		throw WriteFileException(
			"written " + to_string(state.second) + " B out of "
			+ to_string(data.size()) + " B");
	}

	SHA1Engine engine;
	engine.update(data);

	const auto &digest = engine.digest();
	if (state.first != digest) {
		throw WriteFileException(
			"digest '" + DigestEngine::digestToHex(state.first)
			+ "' does not match expected '"
			+ DigestEngine::digestToHex(digest) + "'");
	}

	const auto &name = DigestEngine::digestToHex(state.first);
	writer.commitAs(pathTo(name));
	return name;
}

Journal::Ptr JournalQueuingStrategy::index() const
{
	return m_index;
}

void JournalQueuingStrategy::push(const vector<SensorData> &data)
{
	const string &buffer = FileBuffer::formatEntries(data);
	if (!garbageCollect(buffer.size()))
		dropOldestBuffers(buffer.size());

	const auto &name = writeData(buffer);
	m_index->append(name, "0");
}

size_t JournalQueuingStrategy::readEntries(
		function<void(const Entry &entry)> proc,
		size_t count)
{
	if (count == 0)
		return 0;

	size_t total = 0;

	for (auto it = m_buffers.begin(); it != m_buffers.end();) {
		if (total >= count)
			break;

		if (logger().debug()) {
			logger().debug(
				"reading up to " + to_string(count - total)
				+ " entries from buffer " + it->name(),
				__FILE__, __LINE__);
		}

		total += it->readEntries(proc, count - total);

		if (it->exhausted()) {
			m_exhausted.emplace(it->name(), *it);
			it = m_buffers.erase(it);
			continue;
		}

		poco_assert_msg(total >= count, "total < count && !exhausted");
	}

	return total;
}

size_t JournalQueuingStrategy::precacheEntries(size_t count)
{
	const auto total = readEntries(
		[&](const Entry &entry) {
			m_entryCache.emplace_back(entry);
		},
		count);

	if (logger().debug()) {
		logger().debug(
			"precached " + to_string(total) + " entries, "
			+ to_string(count) + " requested",
			__FILE__, __LINE__);
	}

	return total;
}

bool JournalQueuingStrategy::empty()
{
	if (!m_entryCache.empty())
		return false;

	return precacheEntries(1) == 0;
}

size_t JournalQueuingStrategy::peek(
		vector<SensorData> &data,
		size_t count)
{
	const size_t missingCount = count - m_entryCache.size();
	precacheEntries(missingCount);

	size_t total = 0;

	for (const auto &entry : m_entryCache) {
		if (total >= count)
			break;

		data.emplace_back(entry.data());
		total += 1;
	}

	if (logger().debug()) {
		logger().debug(
			"peek " + to_string(total) + " entries, "
			+ to_string(count) + " requested",
			__FILE__, __LINE__);
	}

	return total;
}

static void updateStatus(
		map<string, size_t> &status,
		const string &buffer,
		size_t nextOffset)
{
	auto result = status.emplace(buffer, nextOffset);
	if (result.second)
		return;

	auto &offset = result.first->second;
	offset = max(offset, nextOffset);
}

void JournalQueuingStrategy::pop(size_t count)
{
	// status to be updated for each buffer
	map<string, size_t> status;

	const size_t cacheCount = min(m_entryCache.size(), count);
	size_t k = 0;

	for (auto it = m_entryCache.begin(); k < cacheCount; ++k, ++it) {
		updateStatus(
			status,
			it->buffer(),
			it->nextOffset());
	}

	const size_t total = cacheCount + readEntries(
		[&](const Entry &entry) {
			updateStatus(
				status,
				entry.buffer(),
				entry.nextOffset());
		},
		(count - cacheCount));

	if (logger().debug()) {
		logger().debug(
			"pop " + to_string(total) + " entries, "
			+ to_string(count) + " requested",
			__FILE__, __LINE__);
	}

	for (const auto &pair : status) {
		if (logger().debug()) {
			logger().debug(
				"buffer " + pair.first + " at offset "
				+ to_string(pair.second),
				__FILE__, __LINE__);
		}

		if (m_exhausted.find(pair.first) == m_exhausted.end()) {
			m_index->append(
				pair.first,
				NumberFormatter::formatHex(pair.second));
		}
		else {
			m_exhausted.erase(pair.first);
			m_index->drop(pair.first);
		}
	}

	k = 0;
	for (auto it = m_entryCache.begin(); k < cacheCount; ++k)
		it = m_entryCache.erase(it);
}

void JournalQueuingStrategy::collectReferenced(set<string> &referenced) const
{
	for (const auto &buffer : m_buffers)
		referenced.emplace(buffer.name());
	for (const auto &pair : m_exhausted)
		referenced.emplace(pair.first);
}

bool JournalQueuingStrategy::garbageCollect(const size_t bytes)
{
	const size_t used = bytesUsedAll();

	if (!overLimit(used + bytes))
		return true;

	if (m_gcDisabled) {
		logger().warning(
			"GC is disabled when over-limit detected: "
			+ to_string(used + bytes) + " B",
			__FILE__, __LINE__);

		return false;
	}

	logger().warning(
		"running GC, over-limit: "
		+ to_string(used + bytes) + " B",
		__FILE__, __LINE__);

	DirectoryIterator it(m_rootDir);
	const DirectoryIterator end;

	set<string> referenced; // collect names of buffers
	collectReferenced(referenced);

	multimap<size_t, File, greater<size_t>> dangling;
	size_t total = 0;

	for (; it != end; ++it) {
		if (!BUFFER_REGEX.match(it.name()))
			continue;

		if (referenced.find(it.name()) != referenced.end())
			continue;

		size_t size = 0;

		try {
			size = it->getSize();
		}
		BEEEON_CATCH_CHAIN_ACTION(logger(),
			continue)

		dangling.emplace(size, *it);
		total += size;
	}

	logger().information(
		"found " + to_string(dangling.size())
		+ " dangling buffer of total size "
		+ to_string(total),
		__FILE__, __LINE__);

	size_t removed = 0;

	while (overLimit(used + bytes - removed)) {
		auto it = dangling.begin();
		if (it == dangling.end())
			break; // no more dangling buffers to remove

		if (whipeFile(it->second))
			removed += it->first;

		dangling.erase(it);
	}

	logger().information(
		"removed " + to_string(removed)
		+ " B of dangling buffers, requested at least "
		+ to_string(bytes) + " B",
		__FILE__, __LINE__);

	return removed >= bytes;
}

void JournalQueuingStrategy::dropOldestBuffers(const size_t bytes)
{
	if (m_neverDropOldest) {
		logger().warning(
			"dropping oldest buffers is disabled while requesting "
			+ to_string(bytes) + " B of space",
			__FILE__, __LINE__);

		return;
	}

	logger().warning(
		"dropping oldest buffers, request " + to_string(bytes) + " B",
		__FILE__, __LINE__);

	set<string> nonDroppable;

	for (const auto &entry : m_entryCache)
		nonDroppable.emplace(entry.buffer());
	for (const auto &pair : m_exhausted)
		nonDroppable.emplace(pair.first);

	list<FileBuffer>::reverse_iterator rbeg = m_buffers.rend();
	size_t space = 0;

	for (auto it = m_buffers.rbegin(); it != m_buffers.rend(); ++it) {
		if (nonDroppable.find(it->name()) != nonDroppable.end()) {
			if (logger().debug()) {
				logger().debug(
					"must not drop " + it->name(),
					__FILE__, __LINE__);
			}

			continue;
		}

		if (rbeg == m_buffers.rend())
			rbeg = it;

		if (logger().debug()) {
			logger().debug(
				"droppable buffer " + it->name()
				+ " (" + to_string(it->size()) + " B)",
				__FILE__, __LINE__);
		}

		space += it->size();

		if (space >= bytes)
			break;
	}

	if (logger().debug()) {
		logger().debug(
			"might drop " + to_string(space) + " B/"
			+ to_string(bytes) + " B",
			__FILE__, __LINE__);
	}

	set<string> dropped;
	size_t removed = 0;

	for (auto it = rbeg; it != m_buffers.rend() && removed < bytes; ++it) {
		if (whipeFile(it->path())) {
			removed += it->size();
			dropped.emplace(it->name());
			m_index->drop(it->name());
		}
	}

	for (const auto &name : dropped) {
		for (auto it = m_buffers.begin(); it != m_buffers.end();) {
			if (it->name() == name)
				it = m_buffers.erase(it);
			else
				++it;
		}
	}

	logger().information(
		"removed " + to_string(removed)
		+ " B of oldest buffers, requested at least "
		+ to_string(bytes) + " B",
		__FILE__, __LINE__);
}

size_t JournalQueuingStrategy::bytesUsed() const
{
	size_t bytes = 0;

	for (const auto &buffer : m_buffers) {
		if (buffer.exhausted())
			continue;

		bytes += buffer.size();
	}

	File index = pathTo("index");
	size_t size = 0;

	try {
		size = index.getSize();
	}
	BEEEON_CATCH_CHAIN(logger())

	return bytes + size;
}

size_t JournalQueuingStrategy::bytesUsedAll() const
{
	size_t bytes = 0;

	DirectoryIterator it(m_rootDir);
	const DirectoryIterator end;

	for (; it != end; ++it) {
		size_t size = 0;

		try {
			size = it->getSize();
		}
		BEEEON_CATCH_CHAIN_ACTION(logger(),
			continue)

		if (logger().debug()) {
			logger().debug(
				"size of " + it.name()
				+ ": " + to_string(size) + " B",
				__FILE__, __LINE__);
		}

		if (BUFFER_REGEX.match(it.name()))
			bytes += size;
		else if (it.name() == "data.tmp")
			bytes += size;
		else if (INDEX_REGEX.match(it.name()))
			bytes += size;
		else if (INDEX_LOCK_REGEX.match(it.name()))
			bytes += size;
	}

	return bytes;
}

Path JournalQueuingStrategy::pathTo(const string &name) const
{
	return Path(m_rootDir, name);
}

string JournalQueuingStrategy::tsString(const Timestamp &t)
{
	return DateTimeFormatter::format(t, DateTimeFormat::ISO8601_FORMAT);
}

JournalQueuingStrategy::Entry::Entry(
		const SensorData &data,
		const string &buffer,
		const size_t nextOffset):
	m_data(data),
	m_buffer(buffer),
	m_nextOffset(nextOffset)
{
}

const SensorData &JournalQueuingStrategy::Entry::data() const
{
	return m_data;
}

string JournalQueuingStrategy::Entry::buffer() const
{
	return m_buffer;
}

size_t JournalQueuingStrategy::Entry::nextOffset() const
{
	return m_nextOffset;
}

void JournalQueuingStrategy::FileBufferStat::update(
		const Timestamp &timestamp)
{
	oldest = min(oldest, timestamp);
	newest = max(newest, timestamp);
}

JournalQueuingStrategy::FileBuffer::FileBuffer(
		const Path &path,
		size_t offset,
		size_t size):
	m_path(path),
	m_offset(offset),
	m_size(size)
{
}

string JournalQueuingStrategy::FileBuffer::name() const
{
	return m_path.getBaseName();
}

Path JournalQueuingStrategy::FileBuffer::path() const
{
	return m_path;
}

size_t JournalQueuingStrategy::FileBuffer::offset() const
{
	return m_offset;
}

size_t JournalQueuingStrategy::FileBuffer::size() const
{
	return m_size;
}

bool JournalQueuingStrategy::FileBuffer::exhausted() const
{
	return m_offset >= m_size;
}

size_t JournalQueuingStrategy::FileBuffer::readEntries(
		function<void(const Entry &entry)> proc)
{
	size_t bytes = 0;

	try {
		const size_t total = scanEntries(m_offset, proc, bytes, 1024);
		m_offset += bytes;
		return total;
	}
	catch (...) {
		m_offset += bytes;
		throw;
	}
}


size_t JournalQueuingStrategy::FileBuffer::readEntries(
		function<void(const Entry &entry)> proc,
		const size_t count)
{
	size_t bytes = 0;

	try {
		const size_t total = scanEntries(m_offset, proc, bytes, count);
		m_offset += bytes;
		return total;
	}
	catch (...) {
		m_offset += bytes;
		throw;
	}
}

void JournalQueuingStrategy::FileBuffer::inspectAndVerify(
	const DigestEngine::Digest &digest,
	FileBufferStat &stat) const
{
	FileInputStream fin(m_path.toString());
	SHA1Engine engine;
	DigestInputStream in(engine, fin);

	while (fin) {
		try {
			scanEntries(
				in,
				[&](const Entry &entry) {
					stat.offset = entry.nextOffset();		
					stat.count += 1;
					stat.update(entry.data().timestamp());
				},
				stat.bytes,
				1024);
		}
		catch (const Exception &e) {
			if (!in.good())
				e.rethrow();

			stat.broken += 1;
		}
	}

	NullOutputStream null;
	StreamCopier::copyStream(in, null);

	const auto &computed = engine.digest();

	if (computed != digest) {
		throw IllegalStateException("digest is invalid: "
			+ DigestEngine::digestToHex(digest)
			+ " != "
			+ DigestEngine::digestToHex(computed));
	}
}

string JournalQueuingStrategy::FileBuffer::formatEntries(
	const vector<SensorData> &data)
{
	static ChecksumSensorDataFormatter formatter(new JSONSensorDataFormatter);

	string buffer;

	for (const auto &one : data) {
		buffer += formatter.format(one);
		buffer += "\n";
	}

	return buffer;
}

size_t JournalQueuingStrategy::FileBuffer::scanEntries(
		size_t offset,
		function<void(const Entry &entry)> proc,
		size_t &bytes,
		const size_t count) const
{
	if (offset >= m_size)
		return 0;

	FileInputStream fin(m_path.toString());
	fin.seekg(offset);

	if (fin.eof())
		return 0;

	if (!fin) {
		throw ReadFileException(
			"failed to seek " + m_path.toString()
			+ " to " + to_string(m_offset));
	}

	return scanEntries(fin, proc, bytes, count);
}

size_t JournalQueuingStrategy::FileBuffer::scanEntries(
		istream &in,
		function<void(const Entry &entry)> proc,
		size_t &bytes,
		const size_t count) const
{
	static ChecksumSensorDataParser parser(new JSONSensorDataParser);

	string line;
	size_t total = 0;

	while (getline(in, line)) {
		if (total >= count)
			break;

		bytes += line.size() + 1;

		if (trim(line).empty())
			continue;

		try {
			proc({
				parser.parse(line),
				name(),
				m_offset + bytes
			});

			total += 1;
		}
		catch (...) {
			break;
		}
	}

	return total;
}
