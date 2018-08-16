#include "zwave/ZWaveNetwork.h"

using namespace std;
using namespace BeeeOn;

ZWaveNetwork::PollEvent::PollEvent():
	m_type(EVENT_NONE)
{
}

ZWaveNetwork::PollEvent::PollEvent(Type type):
	m_type(type)
{
}

ZWaveNetwork::PollEvent::PollEvent(Type type, const ZWaveNode &node):
	m_type(type),
	m_node(new ZWaveNode(node))
{
}

ZWaveNetwork::PollEvent::PollEvent(const ZWaveNode::Value &value):
	m_type(EVENT_VALUE),
	m_value(new ZWaveNode::Value(value))
{
}

ZWaveNetwork::PollEvent::~PollEvent()
{
}

ZWaveNetwork::PollEvent ZWaveNetwork::PollEvent::createNewNode(
		const ZWaveNode &node)
{
	return PollEvent(EVENT_NEW_NODE, node);
}

ZWaveNetwork::PollEvent ZWaveNetwork::PollEvent::createUpdateNode(
		const ZWaveNode &node)
{
	return PollEvent(EVENT_UPDATE_NODE, node);
}

ZWaveNetwork::PollEvent ZWaveNetwork::PollEvent::createRemoveNode(
		const ZWaveNode &node)
{
	return PollEvent(EVENT_REMOVE_NODE, node);
}

ZWaveNetwork::PollEvent ZWaveNetwork::PollEvent::createValue(
		const ZWaveNode::Value &value)
{
	return PollEvent(value);
}

ZWaveNetwork::PollEvent ZWaveNetwork::PollEvent::createInclusionStart()
{
	return PollEvent(EVENT_INCLUSION_START);
}

ZWaveNetwork::PollEvent ZWaveNetwork::PollEvent::createInclusionDone()
{
	return PollEvent(EVENT_INCLUSION_DONE);
}

ZWaveNetwork::PollEvent ZWaveNetwork::PollEvent::createRemoveNodeStart()
{
	return PollEvent(EVENT_REMOVE_NODE_START);
}

ZWaveNetwork::PollEvent ZWaveNetwork::PollEvent::createRemoveNodeDone()
{
	return PollEvent(EVENT_REMOVE_NODE_DONE);
}

ZWaveNetwork::PollEvent ZWaveNetwork::PollEvent::createReady()
{
	return PollEvent(EVENT_READY);
}

ZWaveNetwork::PollEvent::Type ZWaveNetwork::PollEvent::type() const
{
	return m_type;
}

const ZWaveNode &ZWaveNetwork::PollEvent::node() const
{
	return *m_node;
}

const ZWaveNode::Value &ZWaveNetwork::PollEvent::value() const
{
	return *m_value;
}

string ZWaveNetwork::PollEvent::toString() const
{
	switch (m_type) {
	case EVENT_NONE:
		return "event none";
	case EVENT_NEW_NODE:
		return "event new-node " + m_node->toString();
	case EVENT_UPDATE_NODE:
		return "event update-node " + m_node->toString();
	case EVENT_REMOVE_NODE:
		return "event remove-node " + m_node->toString();
	case EVENT_INCLUSION_START:
		return "event inclusion-start";
	case EVENT_INCLUSION_DONE:
		return "event inclusion-done";
	case EVENT_REMOVE_NODE_START:
		return "event remove-node-start";
	case EVENT_REMOVE_NODE_DONE:
		return "event remove-node-done";
	case EVENT_VALUE:
		return "event value " + m_value->toString();
	case EVENT_READY:
		return "event ready";
	}

	return "event unknown";
}

ZWaveNetwork::~ZWaveNetwork()
{
}
