#include <cerrno>
#include <functional>
#include <map>

#include <Poco/Checksum.h>
#include <Poco/Error.h>
#include <Poco/Exception.h>
#include <Poco/FileStream.h>
#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/String.h>

#include "io/SafeWriter.h"
#include "util/Journal.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

static const string OP_DROP = "drop";

Journal::Journal(
		const Poco::Path &file,
		double duplicatesFactor,
		size_t minimalRewritesSize):
	m_file(file),
	m_duplicatesFactor(duplicatesFactor),
	m_minimalRewriteSize(minimalRewritesSize),
	m_dirty(false)
{
	if (m_duplicatesFactor < 1.0)
		throw InvalidArgumentException("duplicatesFactor must be at least 1");
}

Journal::~Journal()
{
	if (!m_dirty.empty()) {
		try {
			appendFlush();
		}
		BEEEON_CATCH_CHAIN(logger())
	}
}

bool Journal::createEmpty()
{
	return File(m_file).createFile();
}

void Journal::checkExisting(bool regularFile, bool writable) const
{
	bool exists = false;
	
	try {
		exists = m_file.exists();
	}
	BEEEON_CATCH_CHAIN(logger())

	if (exists) {
		if (!m_file.canRead()) {
			throw FileAccessDeniedException(
				"cannot read file " + m_file.path());
		}

		if (regularFile && !m_file.isFile()) {
			throw InvalidArgumentException(
				"file " + m_file.path() + " must be a regular file");
		}

		if (writable && !m_file.canWrite()) {
			throw FileReadOnlyException(
				"cannot write file " + m_file.path());
		}
	}
	else {
		File parent(Path(m_file.path()).parent());

		if (!parent.exists()) {
			throw FileNotFoundException(
				"directory " + parent.path() + " does not exist");
		}

		if (!parent.canRead()) {
			throw FileAccessDeniedException(
				"cannot read from directory " + parent.path());
		}

		if (!parent.isDirectory()) {
			throw InvalidArgumentException(
				"directory " + parent.path() + " must be a directory");
		}

		if (writable && !parent.canWrite()) {
			throw FileReadOnlyException(
				"cannot write into directory " + parent.path());
		}
	}
}

void Journal::load(bool recover)
{
	if (m_file.getSize() == 0)
		return;

	FileInputStream fin(m_file.path());
	load(fin, recover);
}

void Journal::load(istream &in, bool recover)
{
	list<Record> records;

	if (recover)
		parseStreamRecover(in, records);
	else
		parseStream(in, records);

	Mutex::ScopedLock guard(m_lock);
	m_records.clear();
	m_records.insert(m_records.end(), records.begin(), records.end());
	m_dirty.clear();
}

void Journal::checkConsistent() const
{
	FileInputStream fin(m_file.path());
	checkConsistent(fin);
}

void Journal::checkConsistent(std::istream &in) const
{
	list<Record> records;
	parseStreamRecover(in, records);
	interpret(records);

	if (records != this->records())
		throw IllegalStateException("inconsistent journals");
}

static void doParseStream(
		istream &in,
		function<void(const string &, size_t)> proc)
{
	string line;
	size_t lineno = 0;

	while (getline(in, line)) {
		++lineno;

		if (trim(line).empty())
			continue;

		proc(line, lineno);
	}
}

void Journal::parseStream(istream &in, list<Record> &records) const
{
	doParseStream(in, [&](const string &line, size_t lineno) {
		records.emplace_back(parse(line, lineno));
	});
}

void Journal::parseStreamRecover(istream &in, list<Record> &records) const
{
	doParseStream(in, [&](const string &line, size_t lineno) {
		try {
			records.emplace_back(parse(line, lineno));
		}
		BEEEON_CATCH_CHAIN(logger())
	});
}

void Journal::append(
		const Record &record,
		bool flush)
{
	checkRecord(record);

	Mutex::ScopedLock guard(m_lock);

	m_dirty.emplace_back(record);

	if (flush)
		this->flush();
}

void Journal::appendDrop(const string &key, bool flush)
{
	Mutex::ScopedLock guard(m_lock);

	m_dirty.emplace_back(Record{key, OP_DROP});

	if (flush)
		this->flush();
}

void Journal::drop(const set<string> &keys, bool flush)
{
	Mutex::ScopedLock guard(m_lock);

	list<Record> records = this->records();

	for (auto it = keys.begin(); it != keys.end();) {
		const auto &key = *it;
		++it;
		appendDrop(key, it == keys.end() ? flush : false);
	}
}

void Journal::flush()
{
	Mutex::ScopedLock guard(m_lock);

	const auto factor = duplicatesFactor(m_records);

	if (factor > m_duplicatesFactor && overMinimalSize())
		interpretAndFlush();
	else
		appendFlush();
}

double Journal::currentDuplicatesFactor() const
{
	Mutex::ScopedLock guard(m_lock);

	return duplicatesFactor(m_records);
}

double Journal::duplicatesFactor(const list<Record> &records) const
{
	set<string> unique;

	for (const auto &r : records)
		unique.emplace(r.key);

	if (unique.size() == 0)
		return 1.0;

	return static_cast<double>(records.size())
		/ static_cast<double>(unique.size());
}

bool Journal::overMinimalSize() const
{
	return bytes(m_records) + bytes(m_dirty) > m_minimalRewriteSize;
}

void Journal::interpret(list<Record> &records) const
{
	map<string, list<Record>::iterator> cache;

	for (auto it = records.begin(); it != records.end();) {
		if (it->value == OP_DROP) {
			auto orig = cache.find(it->key);
			if (orig != cache.end())
				records.erase(orig->second);

			cache.erase(it->key);
			it = records.erase(it);
			continue;
		}

		auto result = cache.emplace(it->key, it);
		if (result.second) {
			++it;
			continue;
		}

		auto orig = result.first->second;
		*orig = *it;
		// remove it as its contents has been moved
		it = records.erase(it);
	}
}

void Journal::interpretAndFlush()
{
	try {
		list<Record> records = recordsRaw();
		interpret(records);
		rewriteAndFlush(records);
	}
	catch (const WriteFileException &e) {
		logger().log(e, __FILE__, __LINE__);
		// try to fallback to simple append
		appendFlush();
	}
}

void Journal::rewriteAndFlush(const list<Record> &records)
{
	if (logger().debug()) {
		logger().debug("rewriting journal into " + m_file.path(),
			__FILE__, __LINE__);
	}

	SafeWriter writer(m_file, "lock");

	for (const auto &one : records) {
		writer.stream() << format(one) << "\n";
		handleFailure(writer.stream());
	}

	writer.commitAs(m_file);

	m_records.clear();
	m_records.insert(m_records.end(), records.begin(), records.end());
	m_dirty.clear();
}

void Journal::appendFlush()
{
	FileOutputStream fout;
	fout.open(m_file.path(), ios::out | ios::app | ios::binary);

	auto it = m_dirty.begin();

	for (; it != m_dirty.end();) {
		fout << format(*it) << "\n";
		handleFailure(fout);

		fout.flush();
		handleFailure(fout);

		m_records.emplace_back(*it);
		it = m_dirty.erase(it);
	}
}

list<Journal::Record> Journal::records() const
{
	list<Journal::Record> records = recordsRaw();
	interpret(records);

	return records;
}

Nullable<string> Journal::operator [](const string &key) const
{
	for (const auto &record : records()) {
		if (record.key == key)
			return record.value;
	}

	Nullable<string> null;
	return null;
}

list<Journal::Record> Journal::recordsRaw() const
{
	Mutex::ScopedLock guard(m_lock);

	if (m_dirty.empty())
		return m_records;

	list<Record> records = m_records;
	records.insert(records.end(), m_dirty.begin(), m_dirty.end());
	return records;
}

list<Journal::Record> &Journal::committed()
{
	Mutex::ScopedLock guard(m_lock);
	return m_records;
}

list<Journal::Record> &Journal::dirty()
{
	Mutex::ScopedLock guard(m_lock);
	return m_dirty;
}

void Journal::checkRecord(const Record &record) const
{
	if (record.key.find("\t") != string::npos)
		throw InvalidArgumentException("record key must not contain <TAB>");

	if (record.value.find("\n") != string::npos)
		throw InvalidArgumentException("record value must not contain <LF>");

	if (record.value == OP_DROP)
		throw InvalidArgumentException("record value must not be '" + OP_DROP + "'");
}

void Journal::handleFailure(ostream &o) const
{
	if (o.bad()) {
		const auto err = Error::last();

		switch (err) {
		case EPERM:
			throw FileAccessDeniedException(Error::getMessage(err), err);
		case EFBIG:
		case EDQUOT:
		case ENOSPC:
		case EIO:
			throw WriteFileException(Error::getMessage(err), err);
		default:
			throw IOException(Error::getMessage(err), err);
		}
	}
	else if (!o) {
		logger().warning("ostream state: "
			+ string(o.good()? "G" : "")
			+ string(o.eof()? "E" : "")
			+ string(o.fail()? "F" : "")
			+ string(o.bad()? "B" : ""));

		o.clear();
	}
}

string Journal::format(const Record &record, bool zeroSum) const
{
	const auto &line = record.key + "\t" + record.value;

	if (zeroSum)
		return "00000000\t" + line;

	Checksum csum(Checksum::TYPE_CRC32);
	csum.update(line);

	return NumberFormatter::formatHex(csum.checksum(), 8) + "\t" + line;
}

Journal::Record Journal::parse(const string &line, size_t lineno) const
{
	const auto sep = line.find("\t");
	if (sep == string::npos) {
		throw InvalidArgumentException(
			"missing <TAB> separator at "
			+ to_string(lineno));
	}

	uint32_t check = 0;
	
	if (!NumberParser::tryParseHex(line.substr(0, sep), check)) {
		throw SyntaxException(
			"expected hexadecimal checksum at "
			+ to_string(lineno));
	}

	const auto &content = line.substr(sep + 1);

	Checksum csum(Checksum::TYPE_CRC32);
	csum.update(content);

	if (csum.checksum() != check) {
		throw IllegalStateException(
			"checksum mismatch: "
			+ NumberFormatter::formatHex(check, 8)
			+ " != "
			+ NumberFormatter::formatHex(csum.checksum(), 8)
			+ " at " + to_string(lineno));
	}

	const auto valueSep = content.find("\t");
	if (valueSep == string::npos)
		throw AssertionViolationException("no <TAB> separator but checksum is valid");

	return {content.substr(0, valueSep), content.substr(valueSep + 1)};
}

size_t Journal::bytes(const list<Record> &records) const
{
	size_t bytes = 0;

	for (const auto &one : records) {
		bytes += format(one, true).size();
		bytes += 1;
	}

	return bytes;
}
