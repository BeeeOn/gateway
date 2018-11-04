#include "iqrf/DPAMessage.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

uint16_t DPAMessage::COORDINATOR_NODE_ADDRESS = 0x0000;
uint16_t DPAMessage::DEFAULT_HWPID = 0xffff;

DPAMessage::DPAMessage(
		DPAMessage::NetworkAddress node,
		uint8_t pNumber,
		uint8_t pCommand,
		uint16_t hwPID,
		const vector<uint8_t> &pData):
	m_nodeAddress(node),
	m_peripheralNumber(pNumber),
	m_peripheralCommand(pCommand),
	m_hwPID(hwPID),
	m_peripheralData(pData)
{
}

void DPAMessage::setNetworkAddress(DPAMessage::NetworkAddress node)
{
	m_nodeAddress = node;
}

uint16_t DPAMessage::networkAddress() const
{
	return m_nodeAddress;
}

void DPAMessage::setPeripheralNumber(uint8_t pNumber)
{
	m_peripheralNumber = pNumber;
}

uint8_t DPAMessage::peripheralNumber() const
{
	return m_peripheralNumber;
}

void DPAMessage::setPeripheralCommand(uint8_t pCommand)
{
	m_peripheralCommand = pCommand;
}

uint8_t DPAMessage::peripheralCommand() const
{
	return m_peripheralCommand;
}

void DPAMessage::setHWPID(uint16_t hwPID)
{
	m_hwPID = hwPID;
}

uint16_t DPAMessage::HWPID() const
{
	return m_hwPID;
}

void DPAMessage::setPeripheralData(const vector<uint8_t> &data)
{
	m_peripheralData = data;
}

vector<uint8_t> DPAMessage::peripheralData() const
{
	return m_peripheralData;
}

DPAMessage::~DPAMessage()
{
}
