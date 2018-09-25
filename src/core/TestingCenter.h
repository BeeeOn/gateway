#pragma once

#include <functional>
#include <list>
#include <map>
#include <vector>

#include <Poco/AtomicCounter.h>
#include <Poco/SharedPtr.h>

#include "core/CommandHandler.h"
#include "core/CommandSender.h"
#include "credentials/FileCredentialsStorage.h"
#include "io/Console.h"
#include "loop/StoppableRunnable.h"
#include "model/DeviceID.h"
#include "model/DeviceDescription.h"
#include "model/ModuleID.h"
#include "util/CryptoConfig.h"
#include "util/Loggable.h"

namespace BeeeOn {

class TestingCenter :
		public CommandSender,
		public CommandHandler,
		public StoppableRunnable,
		protected Loggable {
public:
	typedef std::map<ModuleID, double> DeviceData;

	struct ActionContext {
		ConsoleSession &console;
		std::map<DeviceID, DeviceData> &devices;
		Poco::Mutex &mutex;
		CommandSender &sender;
		const std::vector<std::string> args;
		Poco::SharedPtr<FileCredentialsStorage> credentialsStorage;
		Poco::SharedPtr<CryptoConfig> cryptoConfig;
		std::list<DeviceID> &newDevices;
		std::set<DeviceID> &acceptedDevices;
		std::map<DeviceID, DeviceDescription> &seenDevices;
	};

	/**
	 * Action to be executed when processing console input.
	 */
	typedef std::function<void(ActionContext &context)> Action;

	struct ActionRecord {
		std::string description;
		Action action;
	};

	TestingCenter();

	bool accept(const Command::Ptr cmd) override;
	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

	void run() override;
	void stop() override;

	void setConsole(Poco::SharedPtr<Console> console);
	void setPairedDevices(const std::list<std::string> &ids);
	Poco::SharedPtr<Console> console() const;
	void setCredentialsStorage(Poco::SharedPtr<FileCredentialsStorage> storage);
	void setCryptoConfig(Poco::SharedPtr<CryptoConfig> config);

protected:
	void registerAction(
			const std::string &name,
			const Action action,
			const std::string &description);
	void printHelp(ConsoleSession &session);
	void processLine(ConsoleSession &session, const std::string &line);

private:
	Poco::SharedPtr<Console> m_console;
	std::list<DeviceID> m_newDevices;
	Poco::AtomicCounter m_stop;
	std::map<std::string, ActionRecord> m_action;
	std::map<DeviceID, DeviceData> m_devices;
	Poco::Mutex m_mutex;
	Poco::SharedPtr<FileCredentialsStorage> m_credentialsStorage;
	Poco::SharedPtr<CryptoConfig> m_cryptoConfig;
	std::map<DeviceID, DeviceDescription> m_seenDevices;
	std::set<DeviceID> m_acceptedDevices;
};

}
