#include "zwave/ZWaveNotificationEvent.h"

using namespace BeeeOn;

ZWaveNotificationEvent::ZWaveNotificationEvent(
		const OpenZWave::Notification &notification):
	m_type(notification.GetType()),
	m_valueId(notification.GetValueID()),
	m_byte(notification.GetByte())
{
	switch (m_type) {
	case OpenZWave::Notification::Type_NodeEvent:
	case OpenZWave::Notification::Type_ControllerCommand:
		m_event = notification.GetEvent();
		break;

	default:
		break;
	}
}

OpenZWave::Notification::NotificationType ZWaveNotificationEvent::type() const
{
	return m_type;
}

OpenZWave::ValueID ZWaveNotificationEvent::valueID() const
{
	return m_valueId;
}

uint8_t ZWaveNotificationEvent::byte() const
{
	return m_byte;
}

Poco::Nullable<uint8_t> ZWaveNotificationEvent::event() const
{
	return m_event;
}
