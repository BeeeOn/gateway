#include "iqrf/IQRFEvent.h"

using namespace BeeeOn;
using namespace std;

IQRFEvent::IQRFEvent(DPARequest::Ptr const request)
{
    m_NetworkAddress = request->networkAddress();
    m_pNumber = request->peripheralNumber();
    m_pCommand = request->peripheralCommand();
    m_hwPID = request->HWPID();
    m_peripheralData = request->peripheralData();

    m_direction = 0x00;
}

IQRFEvent::IQRFEvent(DPAResponse::Ptr const response)
{
    m_NetworkAddress = response->networkAddress();
    m_pNumber = response->peripheralNumber();
    m_pCommand = response->peripheralCommand();
    m_hwPID = response->HWPID();
    m_peripheralData = response->peripheralData();

    m_direction = 0x01;
}

uint16_t IQRFEvent::networkAddress() const
{
    return m_NetworkAddress;
}

uint8_t IQRFEvent::peripheralNumber() const
{
    return m_pNumber;
}

uint8_t IQRFEvent::commandCode() const
{
    return m_pCommand;
}

uint16_t IQRFEvent::HWProfile() const
{
    return m_hwPID;
}

vector<uint8_t> IQRFEvent::payload() const
{
    return m_peripheralData;
}

uint8_t IQRFEvent::direction() const
{
    return m_direction;
}

uint8_t IQRFEvent::size() const
{
    return m_peripheralData.size();
}
