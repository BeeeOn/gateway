#include <cmath>

#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "commands/ServerDeviceListResult.h"
#include "commands/ServerLastValueResult.h"
#include "di/Injectable.h"
#include "server/GWSCommandHandler.h"
#include "gwmessage/GWNewDeviceRequest.h"
#include "gwmessage/GWDeviceListRequest.h"
#include "gwmessage/GWLastValueRequest.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GWSCommandHandler)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(GWSListener)
BEEEON_OBJECT_PROPERTY("connector", &GWSCommandHandler::setConnector)
BEEEON_OBJECT_END(BeeeOn, GWSCommandHandler)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

GWSCommandHandler::GWSCommandHandler()
{
}

void GWSCommandHandler::setConnector(GWSConnector::Ptr connector)
{
	m_connector = connector;
}

bool GWSCommandHandler::accept(const Command::Ptr cmd)
{
	if (!cmd.cast<NewDeviceCommand>().isNull())
		return true;
	if (!cmd.cast<ServerDeviceListCommand>().isNull())
		return true;
	if (!cmd.cast<ServerLastValueCommand>().isNull())
		return true;

	return false;
}

void GWSCommandHandler::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	if (!cmd.cast<NewDeviceCommand>().isNull())
		handleNewDevice(cmd.cast<NewDeviceCommand>(), answer);
	else if (!cmd.cast<ServerDeviceListCommand>().isNull())
		handleDeviceList(cmd.cast<ServerDeviceListCommand>(), answer);
	else if (!cmd.cast<ServerLastValueCommand>().isNull())
		handleLastValue(cmd.cast<ServerLastValueCommand>(), answer);
	else
		throw IllegalStateException("command " + cmd->toString() + " cannot be handled");
}

void GWSCommandHandler::handleNewDevice(NewDeviceCommand::Ptr cmd, Answer::Ptr answer)
{
	GWNewDeviceRequest::Ptr request = new GWNewDeviceRequest;
	request->setDeviceDescription(cmd->description());

	sendRequest(request, new Result(answer));
}

void GWSCommandHandler::handleDeviceList(ServerDeviceListCommand::Ptr cmd, Answer::Ptr answer)
{
	GWDeviceListRequest::Ptr request = new GWDeviceListRequest;
	request->setDevicePrefix(cmd->devicePrefix());

	sendRequest(request, new ServerDeviceListResult(answer));
}

void GWSCommandHandler::handleLastValue(ServerLastValueCommand::Ptr cmd, Answer::Ptr answer)
{
	GWLastValueRequest::Ptr request = new GWLastValueRequest;
	request->setDeviceID(cmd->deviceID());
	request->setModuleID(cmd->moduleID());

	ServerLastValueResult::Ptr result = new ServerLastValueResult(answer);
	result->setDeviceID(cmd->deviceID());
	result->setModuleID(cmd->moduleID());

	sendRequest(request, result);
}

void GWSCommandHandler::sendRequest(GWRequest::Ptr request, Result::Ptr result)
{
	request->setID(GlobalID::random());

	if (logger().debug()) {
		logger().debug(
			"sending request " + request->toBriefString(),
			__FILE__, __LINE__);
	}

	try {
		sendRequestUnsafe(request, result);
	}
	BEEEON_CATCH_CHAIN_ACTION(logger(),
		result->setStatus(Result::Status::FAILED))
}

void GWSCommandHandler::sendRequestUnsafe(GWRequest::Ptr request, Result::Ptr result)
{
	FastMutex::ScopedLock guard(m_lock);
	const auto p = m_pending.emplace(request->id(), result);

	if (!p.second) {
		throw IllegalStateException(
			"duplicate request ID: " + request->toBriefString());
	}

	m_connector->send(request);
}

void GWSCommandHandler::onResponse(const GWResponse::Ptr response)
{
	FastMutex::ScopedLock guard(m_lock);

	auto it = m_pending.find(response->id());
	if (it == m_pending.end()) {
		if (logger().debug()) {
			logger().debug(
				"received spurious response "
				+ response->toBriefString(),
				__FILE__, __LINE__);
		}

		return;
	}

	if (response->status() == GWResponse::ACCEPTED) {
		logger().warning(
			"request " + response->toBriefString()
			+ " was accepted, ignoring");
		return;
	}

	Result::Ptr result = it->second;
	m_pending.erase(it);

	if (response->status() == GWResponse::FAILED) {
		logger().notice(
			"request " + response->toBriefString()
			+ " is considered as failed");

		result->setStatus(Result::Status::FAILED);
		return;
	}

	poco_assert(response->status() == GWResponse::SUCCESS);

	try {
		onSpecificResponse(response, result);
		result->setStatus(Result::Status::SUCCESS);
	}
	BEEEON_CATCH_CHAIN_ACTION(logger(),
		result->setStatus(Result::Status::FAILED))
}

void GWSCommandHandler::onSpecificResponse(
		const GWResponse::Ptr response,
		Result::Ptr result)
{
	switch (response->type()) {
	case GWMessageType::DEVICE_LIST_RESPONSE:
		onDeviceListResponse(response.cast<GWDeviceListResponse>(), result);
		break;

	case GWMessageType::LAST_VALUE_RESPONSE:
		onLastValueResponse(response.cast<GWLastValueResponse>(), result);
		break;

	case GWMessageType::GENERIC_RESPONSE:
		// nothing to do, really...
		break;

	default:
		throw IllegalStateException(
			"unrecognized response: " + response->toBriefString());
	}
}

void GWSCommandHandler::onDeviceListResponse(
		const GWDeviceListResponse::Ptr response,
		Result::Ptr result)
{
	poco_assert_msg(!response.isNull(), "expected GWDeviceListResponse");

	ServerDeviceListResult::Ptr specificResult = result.cast<ServerDeviceListResult>();
	poco_assert_msg(!specificResult.isNull(), "expected ServerDeviceListResult");

	ServerDeviceListResult::DeviceValues values;

	for (const auto &id : response->devices())
		values.emplace(id, response->modulesValues(id));

	specificResult->setDevices(values);
}

void GWSCommandHandler::onLastValueResponse(
		const GWLastValueResponse::Ptr response,
		Result::Ptr result)
{
	poco_assert_msg(!response.isNull(), "expected GWLastValueResponse");

	ServerLastValueResult::Ptr specificResult = result.cast<ServerLastValueResult>();
	poco_assert_msg(!specificResult.isNull(), "expected ServerLastValueResult");

	if (response->valid())
		specificResult->setValue(response->value());
	else
		specificResult->setValue(std::nan(""));
}
