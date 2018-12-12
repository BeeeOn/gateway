#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "server/AbstractGWSConnector.h"
#include "util/Occasionally.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

AbstractGWSConnector::AbstractGWSConnector():
	m_outputsCount(2)
{
}

void AbstractGWSConnector::setOutputsCount(int count)
{
	if (count < 1)
		throw InvalidArgumentException("outputsCount must be at least 1");

	m_outputsCount = count;
}

void AbstractGWSConnector::setPriorityAssigner(GWSPriorityAssigner::Ptr assigner)
{
	m_priorityAssigner = assigner;
}

void AbstractGWSConnector::setupQueues()
{
	Mutex::ScopedLock guard(m_outputLock);

	if (logger().debug()) {
		logger().debug(
			"setup " + to_string(m_outputsCount) + " queues"
			__FILE__, __LINE__);
	}

	m_outputs.clear();
	for (unsigned int i = 0; i < m_outputsCount; ++i) {
		m_outputs.emplace_back(queue<GWMessage::Ptr>());
		m_outputsStatus.emplace_back(0);
	}

	poco_assert(m_outputs.size() == m_outputsStatus.size());
}

size_t AbstractGWSConnector::selectOutput() const
{
	static Occasionally occasionally;

	Mutex::ScopedLock guard(m_outputLock);

	vector<bool> go;
	for (size_t i = 0; i < m_outputsStatus.size(); ++i) {
		go.push_back(false);

		size_t others = 0;
		size_t count = 0;

		for (size_t j = i + 1; j < m_outputsStatus.size(); ++j) {
			if (m_outputs[j].empty())
				continue;

			others += m_outputsStatus[j];
			count += 1;
		}

		if (m_outputsStatus[i] <= others || count == 0)
			go[i] = true;
	}

	poco_assert(!go.empty());

	go[go.size() - 1] = true;

	poco_assert(go.size() == m_outputsStatus.size());
	poco_assert(go.size() == m_outputs.size());

	occasionally.execute([&]() {
		string queues = to_string(m_outputs.front().size())
			+ " [" + (go.front()? "*" : "")
			+ to_string(m_outputsStatus.front()) + "]";

		for (size_t i = 1; i < m_outputs.size(); ++i) {
			queues += ", " + to_string(m_outputs[i].size())
				+ " [" + (go[i]? "*" : "")
				+ to_string(m_outputsStatus[i]) + "]";
		}

		logger().information(
			"queues " + queues,
			__FILE__, __LINE__);
	});

	for (size_t i = 0; i < go.size(); ++i) {
		if (!go[i])
			continue;

		auto &queue = m_outputs[i];
		if (queue.empty())
			continue;

		return i;
	}

	return go.size();
}

void AbstractGWSConnector::updateOutputs(size_t i)
{
	Mutex::ScopedLock guard(m_outputLock);

	m_outputsStatus[i] += 1;

	size_t highest = 0;

	for (const auto &status : m_outputsStatus)
		highest = status > highest? status : highest;

	if (highest < 16)
		return;

	for (auto &status : m_outputsStatus)
		status /= 16;
}

bool AbstractGWSConnector::outputValid(size_t i) const
{
	Mutex::ScopedLock guard(m_outputLock);

	return i < m_outputs.size();
}

GWMessage::Ptr AbstractGWSConnector::peekOutput(size_t i) const
{
	Mutex::ScopedLock guard(m_outputLock);

	poco_assert(!m_outputs[i].empty());
	return m_outputs[i].front();
}

void AbstractGWSConnector::popOutput(size_t i)
{
	Mutex::ScopedLock guard(m_outputLock);

	poco_assert(!m_outputs[i].empty());
	m_outputs[i].pop();
}

void AbstractGWSConnector::send(const GWMessage::Ptr message)
{
	const auto priority = m_priorityAssigner->assignPriority(message);

	Mutex::ScopedLock guard(m_outputLock);

	if (logger().debug()) {
		logger().debug(
			"send " + message->toBriefString()
			+ " with priority " + to_string(priority),
			__FILE__, __LINE__);
	}

	if (priority > m_outputs.size())
		m_outputs.back().emplace(message);
	else
		m_outputs[priority].emplace(message);

	m_outputsUpdated.set();
}
