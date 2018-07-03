#include <Poco/DigestEngine.h>
#include <Poco/DigestStream.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>
#include <Poco/FileStream.h>
#include <Poco/NullStream.h>
#include <Poco/RegularExpression.h>
#include <Poco/StreamCopier.h>
#include <Poco/SHA1Engine.h>

#include "di/Injectable.h"
#include "exporters/RecoverableJournalQueuingStrategy.h"
#include "io/SafeWriter.h"

BEEEON_OBJECT_BEGIN(BeeeOn, RecoverableJournalQueuingStrategy)
BEEEON_OBJECT_CASTABLE(QueuingStrategy)
BEEEON_OBJECT_PROPERTY("rootDir", &RecoverableJournalQueuingStrategy::setRootDir)
BEEEON_OBJECT_PROPERTY("disableGC", &RecoverableJournalQueuingStrategy::setDisableGC)
BEEEON_OBJECT_PROPERTY("neverDropOldest", &RecoverableJournalQueuingStrategy::setNeverDropOldest)
BEEEON_OBJECT_PROPERTY("bytesLimit", &RecoverableJournalQueuingStrategy::setBytesLimit)
BEEEON_OBJECT_PROPERTY("ignoreIndexErrors", &RecoverableJournalQueuingStrategy::setIgnoreIndexErrors)
BEEEON_OBJECT_PROPERTY("disableTmpDataRecovery", &RecoverableJournalQueuingStrategy::setDisableTmpDataRecovery)
BEEEON_OBJECT_PROPERTY("disableBrokenRecovery", &RecoverableJournalQueuingStrategy::setDisableBrokenRecovery)
BEEEON_OBJECT_PROPERTY("disableLostRecovery", &RecoverableJournalQueuingStrategy::setDisableLostRecovery)
BEEEON_OBJECT_END(BeeeOn, RecoverableJournalQueuingStrategy)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

static const RegularExpression BUFFER_REGEX("^[a-fA-F0-9]{40}$");

RecoverableJournalQueuingStrategy::RecoverableJournalQueuingStrategy():
	m_disableTmpDataRecovery(false),
	m_disableBrokenRecovery(false),
	m_disableLostRecovery(false)
{
}

void RecoverableJournalQueuingStrategy::setDisableTmpDataRecovery(bool disable)
{
	m_disableTmpDataRecovery = disable;
}

void RecoverableJournalQueuingStrategy::setDisableBrokenRecovery(bool disable)
{
	m_disableBrokenRecovery = disable;
}

void RecoverableJournalQueuingStrategy::setDisableLostRecovery(bool disable)
{
	m_disableLostRecovery = disable;
}

void RecoverableJournalQueuingStrategy::setup()
{
	File indexFile = pathTo("index");
	Timestamp modified;

	try {
		modified = indexFile.getLastModified();
	}
	catch (const FileNotFoundException &) {
		modified.update();
	}

	whipeFile(pathTo("recover.tmp"), true);

	auto newest = JournalQueuingStrategy::initIndexAndScan(
		[&](const string &name, size_t, Timestamp &newest)
		{
			logger().warning(
				"buffer " + name + " is broken",
				__FILE__, __LINE__);

			recoverBroken(name, newest);
		}
	);

	recoverTmpData(newest);

	list<File> recoverable;
	collectRecoverable(recoverable);
	recoverLost(recoverable, modified, newest);

	reportStats(newest);
}

void RecoverableJournalQueuingStrategy::collectRecoverable(list<File> &files) const
{
	DirectoryIterator it(rootDir());
	const DirectoryIterator end;

	set<string> referenced; // collect names of active buffers
	collectReferenced(referenced);

	for (; it != end; ++it) {
		if (!BUFFER_REGEX.match(it.name()))
			continue; // non-buffer, skip

		if (referenced.find(it.name()) != referenced.end())
			continue; // we know this buffer

		files.emplace_back(*it);
	}
}

size_t RecoverableJournalQueuingStrategy::recoverEntries(
		File file,
		vector<SensorData> &data) const
{
	FileBuffer buffer(file.path(), 0, file.getSize());
	size_t lastOffset = ~((size_t) 0);
	size_t errors = 0;

	while (lastOffset != buffer.offset()) {
		try {
			buffer.readEntries(
				[&](const Entry &entry) {
					data.emplace_back(entry.data());
				});
		}
		catch (const IOException &e) {
			logger().log(e, __FILE__, __LINE__);
			break;
		}
		catch (const OutOfMemoryException &e) {
			logger().log(e, __FILE__, __LINE__);
			break;
		}
		catch (const bad_alloc &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
			break;
		}
		catch (...) {
			errors += 1;
		}

		lastOffset = buffer.offset();
	}

	return errors;
}

string RecoverableJournalQueuingStrategy::recoverBrokenBuffer(File file) const
{
	logger().debug(
		"recovering broken buffer at " + file.path(),
		__FILE__, __LINE__);

	vector<SensorData> tmp;
	const auto errors = recoverEntries(file, tmp);

	if (tmp.size() == 0) {
		logger().information(
			"file " + file.path()
			+ " seems to be empty, seen "
			+ to_string(errors) + " errors, removing",
			__FILE__, __LINE__);

		whipeFile(file);
		return "";
	}

	SafeWriter writer(pathTo("recover.tmp"));
	writer.stream(true) << FileBuffer::formatEntries(tmp);

	const auto &state = writer.finalize();
	const auto &name = DigestEngine::digestToHex(state.first);

	if (name != Path(file.path()).getBaseName()) {
		writer.commitAs(pathTo(name));
		whipeFile(file);

		logger().warning(
			"recovered " + to_string(tmp.size()) + " entries from "
			+ file.path() + " as " + name
			+ ", seen " + to_string(errors) + " errors",
			__FILE__, __LINE__);
	}
	else {
		writer.reset();

		logger().debug(
			"no recovery needed for " + name + ", the existing file is valid ("
			+ to_string(tmp.size()) + " entries, " + to_string(errors) + " errors)",
			__FILE__, __LINE__);
	}

	return name;
}

string RecoverableJournalQueuingStrategy::recoverBuffer(File tmpFile) const
{
	SHA1Engine engine;
	FileInputStream fin(tmpFile.path());
	DigestInputStream in(engine, fin);
	NullOutputStream null;
	StreamCopier::copyStream(in, null);

	const auto &digest = DigestEngine::digestToHex(engine.digest());
	const auto commitedPath = pathTo(digest);

	if (commitedPath.toString() != tmpFile.path()) {
		logger().debug(
			"fixing file name of " + tmpFile.path()
			+ " to " + commitedPath.toString(),
			__FILE__, __LINE__);

		try {
			tmpFile.renameTo(commitedPath.toString());
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
			whipeFile(tmpFile);
		}
	}

	return recoverBrokenBuffer(commitedPath);
}

void RecoverableJournalQueuingStrategy::recoverTmpData(
		Timestamp &newest)
{
	if (m_disableTmpDataRecovery) {
		logger().notice(
			"recovery of data.tmp is disabled",
			__FILE__, __LINE__);
		return;
	}

	try {
		File tmpData(pathTo("data.tmp"));
		if (!tmpData.exists()) {
			if (logger().debug()) {
				logger().debug("no tmp data file found",
					__FILE__, __LINE__);
			}

			return;
		}

		logger().warning(
			"recovering tmp data file " + tmpData.path(),
			__FILE__, __LINE__);

		const auto &name = recoverBuffer(tmpData);
		index()->append(name, "0");
		inspectAndRegisterBuffer(name, 0, newest);
	}
	BEEEON_CATCH_CHAIN(logger())

	return;
}

void RecoverableJournalQueuingStrategy::recoverBroken(
	const string &broken,
	Timestamp &newest)
{
	if (m_disableBrokenRecovery) {
		logger().notice(
			"recovery of broken buffers ("
			+ to_string(broken.size()) + ") is disabled",
			__FILE__, __LINE__);
		return;
	}

	try {
		const auto &recovered = recoverBrokenBuffer(pathTo(broken));

		if (broken == recovered) {
			logger().warning(
				"recovered broken buffer to the same digest: "
				+ broken + ", seems like an I/O issue - dropping",
				__FILE__, __LINE__);

			index()->drop(broken, false);
		}
		else {
			index()->append(recovered, "0", false);
			index()->drop(broken);

			inspectAndRegisterBuffer(recovered, 0, newest);
		}
	}
	BEEEON_CATCH_CHAIN(logger())
}

void RecoverableJournalQueuingStrategy::recoverLost(
		list<File> &recoverable,
		const Timestamp &indexModified,
		const Timestamp &newest)
{
	if (m_disableLostRecovery) {
		logger().notice(
			"recovery of lost buffers is disabled",
			__FILE__, __LINE__);

		return;
	}

	for (auto it = recoverable.begin(); it != recoverable.end();) {
		File &file = *it;
		const Path path(it->path());

		try {
			const auto modified = file.getLastModified();
			if (modified < indexModified) {
				++it;
				continue;
			}

			FileBuffer buffer(file.path(), 0, file.getSize());
			FileBufferStat stat;

			buffer.inspectAndVerify(
				DigestEngine::digestFromHex(path.getBaseName()),
				stat);

			if (stat.oldest >= newest) {
				logger().warning(
					"discovered a potentially lost buffer "
					+ buffer.name() + " with period "
					+ tsString(stat.oldest) + ".." + tsString(stat.newest)
					+ " newer than " + tsString(newest),
					__FILE__, __LINE__);

				index()->append(buffer.name(), "0");
				registerBuffer(buffer, stat);

				it = recoverable.erase(it);
			}
		}
		BEEEON_CATCH_CHAIN(logger())

		++it;
	}
}
