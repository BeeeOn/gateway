#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "bluetooth/HciInfo.h"

using namespace std;
using namespace BeeeOn;

HciInfo::HciInfo(const struct hci_dev_info &info):
	m_name(info.name),
	m_address(info.bdaddr.b),
	m_aclMtu(info.acl_mtu),
	m_scoMtu(info.sco_mtu),
	m_aclPackets(info.acl_pkts),
	m_scoPackets(info.sco_pkts),
	m_rxErrors(info.stat.err_rx),
	m_txErrors(info.stat.err_tx),
	m_rxEvents(info.stat.evt_rx),
	m_txCmds(info.stat.cmd_tx),
	m_rxAcls(info.stat.acl_rx),
	m_txAcls(info.stat.acl_tx),
	m_rxScos(info.stat.sco_rx),
	m_txScos(info.stat.sco_tx),
	m_rxBytes(info.stat.byte_rx),
	m_txBytes(info.stat.byte_tx)
{
}

string HciInfo::name() const
{
	return m_name;
}

MACAddress HciInfo::address() const
{
	return m_address;
}

uint32_t HciInfo::aclMtu() const
{
	return m_aclMtu;
}

uint32_t HciInfo::aclPackets() const
{
	return m_aclPackets;
}

uint32_t HciInfo::scoMtu() const
{
	return m_scoMtu;
}

uint32_t HciInfo::scoPackets() const
{
	return m_scoPackets;
}

uint32_t HciInfo::rxErrors() const
{
	return m_rxErrors;
}

uint32_t HciInfo::txErrors() const
{
	return m_txErrors;
}

uint32_t HciInfo::rxEvents() const
{
	return m_rxEvents;
}

uint32_t HciInfo::txCmds() const
{
	return m_txCmds;
}

uint32_t HciInfo::rxAcls() const
{
	return m_rxAcls;
}

uint32_t HciInfo::txAcls() const
{
	return m_txAcls;
}

uint32_t HciInfo::rxScos() const
{
	return m_rxScos;
}

uint32_t HciInfo::txScos() const
{
	return m_txScos;
}

uint32_t HciInfo::rxBytes() const
{
	return m_rxBytes;
}

uint32_t HciInfo::txBytes() const
{
	return m_txBytes;
}
