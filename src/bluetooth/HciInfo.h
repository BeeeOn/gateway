#ifndef BEEEON_HCI_INFO_H
#define BEEEON_HCI_INFO_H

#include <string>

#include "net/MACAddress.h"

// forward-declaration
struct hci_dev_info;

namespace BeeeOn {

/**
 * Provides information about a hci interface.
 */
class HciInfo {
public:
	HciInfo(const struct hci_dev_info &info);

	std::string name() const;
	MACAddress address() const;

	uint32_t aclMtu() const;
	uint32_t aclPackets() const;

	uint32_t scoMtu() const;
	uint32_t scoPackets() const;

	uint32_t rxErrors() const;
	uint32_t txErrors() const;

	uint32_t rxEvents() const;
	uint32_t txCmds() const;

	uint32_t rxAcls() const;
	uint32_t txAcls() const;

	uint32_t rxScos() const;
	uint32_t txScos() const;

	uint32_t rxBytes() const;
	uint32_t txBytes() const;

private:
	std::string m_name;
	MACAddress m_address;
	uint32_t m_aclMtu;
	uint32_t m_scoMtu;
	uint32_t m_aclPackets;
	uint32_t m_scoPackets;
	uint32_t m_rxErrors;
	uint32_t m_txErrors;
	uint32_t m_rxEvents;
	uint32_t m_txCmds;
	uint32_t m_rxAcls;
	uint32_t m_txAcls;
	uint32_t m_rxScos;
	uint32_t m_txScos;
	uint32_t m_rxBytes;
	uint32_t m_txBytes;
};

}

#endif
