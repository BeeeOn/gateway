#include <Poco/Exception.h>

#include "belkin/BelkinWemoDevice.h"

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::XML;
using namespace std;

BelkinWemoDevice::BelkinWemoDevice()
{
}

BelkinWemoDevice::BelkinWemoDevice(const DeviceID& id):
	m_deviceId(id)
{
}

BelkinWemoDevice::~BelkinWemoDevice()
{
}

DeviceID BelkinWemoDevice::deviceID() const
{
	return m_deviceId;
}

FastMutex& BelkinWemoDevice::lock()
{
	return m_lock;
}

Node* BelkinWemoDevice::findNode(NodeIterator& iterator, const string& name)
{
	Node* xmlNode = iterator.nextNode();

	while (xmlNode) {
		if (xmlNode->nodeName() == name) {
			xmlNode = iterator.nextNode();
			if (xmlNode == NULL)
				return NULL;
			else if (xmlNode->nodeName() != "#text")
				return NULL;
			else
				return xmlNode;
		}

		xmlNode = iterator.nextNode();
	}

	throw NotFoundException("node " + name + " in XML message from belkin device not found");
}

list<Node*> BelkinWemoDevice::findNodes(NodeIterator& iterator, const string& name)
{
	Node* xmlNode = iterator.nextNode();
	list<Node*> list;

	while (xmlNode) {
		if (xmlNode->nodeName() == name) {
			xmlNode = iterator.nextNode();
			if (xmlNode == NULL)
				continue;
			else if (xmlNode->nodeName() == "#text")
				list.push_back(xmlNode);
		}

		xmlNode = iterator.nextNode();
	}

	return list;
}
