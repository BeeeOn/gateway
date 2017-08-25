#include <Poco/Path.h>
#include <Poco/Timer.h>
#include <Poco/Util/AbstractConfiguration.h>

#include "FileCredentialsStorage.h"
#include "util/ConfigurationLoader.h"
#include "util/ConfigurationSaver.h"
#include "di/Injectable.h"

BEEEON_OBJECT_BEGIN(BeeeOn, FileCredentialsStorage)
BEEEON_OBJECT_CASTABLE(CredentialsStorage)
BEEEON_OBJECT_TEXT("file", &FileCredentialsStorage::setFile)
BEEEON_OBJECT_TEXT("configurationRoot", &FileCredentialsStorage::setConfigRoot)
BEEEON_OBJECT_TIME("saveDelayTime", &FileCredentialsStorage::setSaveDelay)
BEEEON_OBJECT_HOOK("done", &FileCredentialsStorage::load)
BEEEON_OBJECT_END(BeeeOn, FileCredentialsStorage)

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Util;
using namespace std;

FileCredentialsStorage::FileCredentialsStorage():
	m_confRoot("credentials"),
	m_callback(*this, &FileCredentialsStorage::onSaveLater),
	m_timerRunning(false),
	m_saveDelayTime(30 * Timespan::MINUTES)
{
}

FileCredentialsStorage::~FileCredentialsStorage()
{
	try {
		m_timer.stop();
		save();
	}
	catch (const Exception &e) {
		logger().log(e, __FILE__, __LINE__);
	}
	catch (exception &e) {
		poco_critical(logger(), e.what());
	}
	catch(...) {
		logger().critical("failed to save credentials into " + m_file,
			__FILE__, __LINE__);
	}
}

void FileCredentialsStorage::setSaveDelay(const Timespan &delay)
{
	if (delay >= 0 && delay.totalSeconds() == 0)
		throw InvalidArgumentException("saveDelay must be negative or at least 1 second");

	RWLock::ScopedWriteLock guard(lock());
	if (delay < 0 && m_timerRunning) {
		m_timer.stop();
		m_timerRunning = false;
	}

	m_saveDelayTime = delay;
}

void FileCredentialsStorage::setFile(const string &path)
{
	m_file = path;
}

void FileCredentialsStorage::setConfigRoot(const string &root)
{
	m_confRoot = root;
}

void FileCredentialsStorage::insertOrUpdate(
		const DeviceID &device,
		const SharedPtr<Credentials> credentials)
{
	RWLock::ScopedWriteLock guard(lock());
	insertOrUpdateUnlocked(device, credentials);
	saveLater();
}

void FileCredentialsStorage::remove(const DeviceID &device)
{
	RWLock::ScopedWriteLock guard(lock());
	removeUnlocked(device);
	saveLater();
}

void FileCredentialsStorage::clear()
{
	RWLock::ScopedWriteLock guard(lock());
	clearUnlocked();
	saveLater();
}

void FileCredentialsStorage::load()
{
	if (m_file.empty())
		return;

	try {
		ConfigurationLoader loader;
		loader.load(Path(m_file));
		loader.finished();
		CredentialsStorage::load(loader.config(), m_confRoot);
	} catch (const IOException &e) {
		logger().log(e, __FILE__, __LINE__);
		logger().warning("could not load credentials due to an I/O error",
			__FILE__, __LINE__);
	}
}

void FileCredentialsStorage::save()
{
	RWLock::ScopedWriteLock guard(lock());

	if (m_timerRunning) {
		m_timer.stop();
		m_timerRunning = false;
	}
	saveUnlocked();
}

void FileCredentialsStorage::saveUnlocked() const
{
	ConfigurationSaver saver(m_file);
	AutoPtr<AbstractConfiguration> conf = saver.config();
	CredentialsStorage::save(conf, m_confRoot);
	saver.save();
	poco_information(logger(), "credentials saved");
}

void FileCredentialsStorage::saveLater()
{
	// No race-condition is possible here because the saveLater is always
	// called while holding the write-lock.
	if (!m_timerRunning && m_saveDelayTime >= Timespan(0)) {
		m_timerRunning = true;
		try {
			m_timer.stop();

			logger().debug("save scheduled in " + to_string(m_saveDelayTime.totalSeconds()) + " s");

			m_timer.setStartInterval(m_saveDelayTime.totalMilliseconds());
			m_timer.setPeriodicInterval(0);
			m_timer.start(m_callback);
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
			m_timerRunning = false;
		}
		catch (exception &e) {
			poco_critical(logger(), e.what());
			m_timerRunning = false;
		}
		catch (...) {
			poco_critical(logger(), "unknown error occured while starting timer");
			m_timerRunning = false;
		}
	}
}

void FileCredentialsStorage::onSaveLater(Timer &)
{
	logger().debug("attempt to autosave");

	try {
		RWLock::ScopedReadLock guard(lock());
		saveUnlocked();
		m_timerRunning = false;
	}
	catch (const Exception &e) {
		logger().log(e, __FILE__, __LINE__);
		m_timerRunning = false;
	}
	catch (exception &e) {
		poco_critical(logger(), e.what());
		m_timerRunning = false;
	}
	catch (...) {
		poco_critical(logger(), "unknown error occured while saving");
		m_timerRunning = false;
	}
}
