#pragma once
#include <Poco/Timer.h>
#include <Poco/AtomicCounter.h>
#include <Poco/Timespan.h>

#include "credentials/CredentialsStorage.h"

namespace BeeeOn{

/**
 * FileCredentialsStorage is subclass of CredentialsStorage which includes
 * methods for saving credentials to file and loading them from it.
 * To load from file, it is necessary to setFile and optionally
 * to setConfigRoot, then call load.
 */
class FileCredentialsStorage : public CredentialsStorage {
public:
	FileCredentialsStorage();
	~FileCredentialsStorage();

	void setFile(const std::string &path);

	/**
	 * Credentials are saved in configuration in this form:
	 * <configRoot>.<DeviceID>.<attribute> = <value>.
	 * Default configRoot is "credentials".
	 */
	void setConfigRoot(const std::string &root);

	/**
	 * If a change occurs in Credentials (inserting, updating, or removing),
	 * FileCredentialsStorage will be automaticly saved after SaveDelayTime
	 * (from the first change).
	 * Default SaveDelayTime is 30min.
	 * Passing negative number to this funcion causes disabling of autosave (if
	 * autosave timer is already running, storage is saved).
	 */
	void setSaveDelay(int seconds);
	void load();
	void save();

	void insertOrUpdate(
		const DeviceID &device,
		const Poco::SharedPtr<Credentials> credentials) override;

	void remove(const DeviceID &device) override;
	void clear() override;

protected:
	/**
	 * This method must be always called while holding the write-lock.
	 */
	void saveLater();
	void onSaveLater(Poco::Timer &);

	void saveUnlocked() const;

private:
	std::string m_file;
	std::string m_confRoot;
	Poco::Timer m_timer;
	Poco::TimerCallback<FileCredentialsStorage> m_callback;
	Poco::AtomicCounter m_timerRunning;
	Poco::Timespan m_saveDelayTime;
};

}
