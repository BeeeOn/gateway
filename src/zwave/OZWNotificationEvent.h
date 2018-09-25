#pragma once

#include <Notification.h>

#include <Poco/Nullable.h>

namespace BeeeOn {

/**
 * @brief Low-level OpenZWave notification event. Because, the OpenZWave::Notification
 * cannot be copied nor cloned, we have to represent it explicitly and copy all its
 * contents.
 *
 * @see https://github.com/OpenZWave/open-zwave/blob/master/cpp/src/Notification.h
 */
class OZWNotificationEvent {
public:
	/**
	 * Copy contents of the given notification into the event
	 * representation.
	 */
	OZWNotificationEvent(const OpenZWave::Notification &notification);

	/**
	 * Shortcut to access home ID.
	 */
	uint32_t homeID() const
	{
		return m_valueId.GetHomeId();
	}

	/**
	 * Shortcut to access node ID.
	 */
	uint8_t nodeID() const
	{
		return m_valueId.GetNodeId();
	}

	/**
	 * @returns type of notification.
	 */
	OpenZWave::Notification::NotificationType type() const;

	/**
	 * @returns identification of the reported value.
	 * @see OpenZWave::ValueID
	 */
	OpenZWave::ValueID valueID() const;

	/**
	 * Note different semantics at least for notifications of
	 * types Type_SceneEvent, Type_Notification, Type_ControllerCommand,
	 * Type_CreateButton, Type_DeleteButton, Type_ButtonOn, Type_ButtonOff,
	 * Type_Group.
	 *
	 * @returns byte value with semantics specific for each type.
	 * @see OpenZWave::Notification
	 */
	uint8_t byte() const;

	/**
	 * The result is valid only for types Type_NodeEvent
	 * and Type_ControllerCommand.
	 *
	 * @returns event as reported by notifications of types
	 * NodeEvent and ControllerCommand.
	 * @see OpenZWave::Notification
	 */
	Poco::Nullable<uint8_t> event() const;

private:
	OpenZWave::Notification::NotificationType m_type;
	OpenZWave::ValueID m_valueId;
	uint8_t m_byte;
	Poco::Nullable<uint8_t> m_event;
};

}
