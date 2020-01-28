/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/
// Tables are indexed by field ID
#include "fields.h"

char *ur_field_names_static[] = {
(char *)   "ACK_COUNT",
(char *)   "ACK_WAITING",
(char *)   "ACL_MTU",
(char *)   "ACL_PACKETS",
(char *)   "ADDRESS",
(char *)   "AVERAGE",
(char *)   "AVERAGE_REQUEST_RTT",
(char *)   "AVERAGE_RESPONSE_RTT",
(char *)   "BAD_CHECKSUM",
(char *)   "BAD_ROUTES",
(char *)   "BROADCAST_READ_COUNT",
(char *)   "BROADCAST_WRITE_COUNT",
(char *)   "BYTE",
(char *)   "CALLBACKS",
(char *)   "CAN_COUNT",
(char *)   "CMDCLASS",
(char *)   "DEV_ADDR",
(char *)   "DROPPED",
(char *)   "EVENT_TYPE",
(char *)   "GENRE",
(char *)   "GW_ID",
(char *)   "HOME_ID",
(char *)   "INDEX",
(char *)   "INSTANCE",
(char *)   "LAST_REQUEST_RTT",
(char *)   "LAST_RESPONSE_RTT",
(char *)   "MESSAGE_TYPE",
(char *)   "NAK_COUNT",
(char *)   "NET_BUSY",
(char *)   "NO_ACK",
(char *)   "NODE_ID",
(char *)   "NON_DELIVERY",
(char *)   "NOT_IDLE",
(char *)   "OOF_COUNT",
(char *)   "QUALITY",
(char *)   "READ_ABORTS",
(char *)   "READ_COUNT",
(char *)   "RECEIVED_COUNT",
(char *)   "RECEIVE_DUPLICATIONS",
(char *)   "RECEIVE_UNSOLICITED",
(char *)   "RETRIES",
(char *)   "ROUTED_BUSY",
(char *)   "RSSI",
(char *)   "RX_ACLS",
(char *)   "RX_BYTES",
(char *)   "RX_ERRORS",
(char *)   "RX_EVENTS",
(char *)   "RX_SCOS",
(char *)   "SCO_MTU",
(char *)   "SCO_PACKETS",
(char *)   "SENT_COUNT",
(char *)   "SENT_FAILED",
(char *)   "SIZE",
(char *)   "SOF_COUNT",
(char *)   "TIMESTAMP",
(char *)   "TX_ACLS",
(char *)   "TX_BYTES",
(char *)   "TX_CMDS",
(char *)   "TX_ERRORS",
(char *)   "TX_SCOS",
(char *)   "TYPE",
(char *)   "VALUE",
(char *)   "WRITE_COUNT",
(char *)   "CHANNELS",
(char *)   "CMD",
(char *)   "EVENT",
(char *)   "MSG_TYPE",
(char *)   "PAYLOAD",
(char *)   "PROT_STATE",
};
short ur_field_sizes_static[] = {
   8, /* ACK_COUNT */
   8, /* ACK_WAITING */
   8, /* ACL_MTU */
   8, /* ACL_PACKETS */
   8, /* ADDRESS */
   8, /* AVERAGE */
   8, /* AVERAGE_REQUEST_RTT */
   8, /* AVERAGE_RESPONSE_RTT */
   8, /* BAD_CHECKSUM */
   8, /* BAD_ROUTES */
   8, /* BROADCAST_READ_COUNT */
   8, /* BROADCAST_WRITE_COUNT */
   8, /* BYTE */
   8, /* CALLBACKS */
   8, /* CAN_COUNT */
   8, /* CMDCLASS */
   8, /* DEV_ADDR */
   8, /* DROPPED */
   8, /* EVENT_TYPE */
   8, /* GENRE */
   8, /* GW_ID */
   8, /* HOME_ID */
   8, /* INDEX */
   8, /* INSTANCE */
   8, /* LAST_REQUEST_RTT */
   8, /* LAST_RESPONSE_RTT */
   8, /* MESSAGE_TYPE */
   8, /* NAK_COUNT */
   8, /* NET_BUSY */
   8, /* NO_ACK */
   8, /* NODE_ID */
   8, /* NON_DELIVERY */
   8, /* NOT_IDLE */
   8, /* OOF_COUNT */
   8, /* QUALITY */
   8, /* READ_ABORTS */
   8, /* READ_COUNT */
   8, /* RECEIVED_COUNT */
   8, /* RECEIVE_DUPLICATIONS */
   8, /* RECEIVE_UNSOLICITED */
   8, /* RETRIES */
   8, /* ROUTED_BUSY */
   8, /* RSSI */
   8, /* RX_ACLS */
   8, /* RX_BYTES */
   8, /* RX_ERRORS */
   8, /* RX_EVENTS */
   8, /* RX_SCOS */
   8, /* SCO_MTU */
   8, /* SCO_PACKETS */
   8, /* SENT_COUNT */
   8, /* SENT_FAILED */
   8, /* SIZE */
   8, /* SOF_COUNT */
   8, /* TIMESTAMP */
   8, /* TX_ACLS */
   8, /* TX_BYTES */
   8, /* TX_CMDS */
   8, /* TX_ERRORS */
   8, /* TX_SCOS */
   8, /* TYPE */
   8, /* VALUE */
   8, /* WRITE_COUNT */
   -1, /* CHANNELS */
   -1, /* CMD */
   -1, /* EVENT */
   -1, /* MSG_TYPE */
   -1, /* PAYLOAD */
   -1, /* PROT_STATE */
};
ur_field_type_t ur_field_types_static[] = {
   UR_TYPE_DOUBLE, /* ACK_COUNT */
   UR_TYPE_DOUBLE, /* ACK_WAITING */
   UR_TYPE_DOUBLE, /* ACL_MTU */
   UR_TYPE_DOUBLE, /* ACL_PACKETS */
   UR_TYPE_DOUBLE, /* ADDRESS */
   UR_TYPE_DOUBLE, /* AVERAGE */
   UR_TYPE_DOUBLE, /* AVERAGE_REQUEST_RTT */
   UR_TYPE_DOUBLE, /* AVERAGE_RESPONSE_RTT */
   UR_TYPE_DOUBLE, /* BAD_CHECKSUM */
   UR_TYPE_DOUBLE, /* BAD_ROUTES */
   UR_TYPE_DOUBLE, /* BROADCAST_READ_COUNT */
   UR_TYPE_DOUBLE, /* BROADCAST_WRITE_COUNT */
   UR_TYPE_DOUBLE, /* BYTE */
   UR_TYPE_DOUBLE, /* CALLBACKS */
   UR_TYPE_DOUBLE, /* CAN_COUNT */
   UR_TYPE_DOUBLE, /* CMDCLASS */
   UR_TYPE_UINT64, /* DEV_ADDR */
   UR_TYPE_DOUBLE, /* DROPPED */
   UR_TYPE_DOUBLE, /* EVENT_TYPE */
   UR_TYPE_DOUBLE, /* GENRE */
   UR_TYPE_DOUBLE, /* GW_ID */
   UR_TYPE_DOUBLE, /* HOME_ID */
   UR_TYPE_DOUBLE, /* INDEX */
   UR_TYPE_DOUBLE, /* INSTANCE */
   UR_TYPE_DOUBLE, /* LAST_REQUEST_RTT */
   UR_TYPE_DOUBLE, /* LAST_RESPONSE_RTT */
   UR_TYPE_DOUBLE, /* MESSAGE_TYPE */
   UR_TYPE_DOUBLE, /* NAK_COUNT */
   UR_TYPE_DOUBLE, /* NET_BUSY */
   UR_TYPE_DOUBLE, /* NO_ACK */
   UR_TYPE_DOUBLE, /* NODE_ID */
   UR_TYPE_DOUBLE, /* NON_DELIVERY */
   UR_TYPE_DOUBLE, /* NOT_IDLE */
   UR_TYPE_DOUBLE, /* OOF_COUNT */
   UR_TYPE_DOUBLE, /* QUALITY */
   UR_TYPE_DOUBLE, /* READ_ABORTS */
   UR_TYPE_DOUBLE, /* READ_COUNT */
   UR_TYPE_DOUBLE, /* RECEIVED_COUNT */
   UR_TYPE_DOUBLE, /* RECEIVE_DUPLICATIONS */
   UR_TYPE_DOUBLE, /* RECEIVE_UNSOLICITED */
   UR_TYPE_DOUBLE, /* RETRIES */
   UR_TYPE_DOUBLE, /* ROUTED_BUSY */
   UR_TYPE_DOUBLE, /* RSSI */
   UR_TYPE_DOUBLE, /* RX_ACLS */
   UR_TYPE_DOUBLE, /* RX_BYTES */
   UR_TYPE_DOUBLE, /* RX_ERRORS */
   UR_TYPE_DOUBLE, /* RX_EVENTS */
   UR_TYPE_DOUBLE, /* RX_SCOS */
   UR_TYPE_DOUBLE, /* SCO_MTU */
   UR_TYPE_DOUBLE, /* SCO_PACKETS */
   UR_TYPE_DOUBLE, /* SENT_COUNT */
   UR_TYPE_DOUBLE, /* SENT_FAILED */
   UR_TYPE_DOUBLE, /* SIZE */
   UR_TYPE_DOUBLE, /* SOF_COUNT */
   UR_TYPE_TIME, /* TIMESTAMP */
   UR_TYPE_DOUBLE, /* TX_ACLS */
   UR_TYPE_DOUBLE, /* TX_BYTES */
   UR_TYPE_DOUBLE, /* TX_CMDS */
   UR_TYPE_DOUBLE, /* TX_ERRORS */
   UR_TYPE_DOUBLE, /* TX_SCOS */
   UR_TYPE_DOUBLE, /* TYPE */
   UR_TYPE_DOUBLE, /* VALUE */
   UR_TYPE_DOUBLE, /* WRITE_COUNT */
   UR_TYPE_STRING, /* CHANNELS */
   UR_TYPE_STRING, /* CMD */
   UR_TYPE_STRING, /* EVENT */
   UR_TYPE_STRING, /* MSG_TYPE */
   UR_TYPE_BYTES, /* PAYLOAD */
   UR_TYPE_STRING, /* PROT_STATE */
};
ur_static_field_specs_t UR_FIELD_SPECS_STATIC = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, 69};
ur_field_specs_t ur_field_specs = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, 69, 69, 69, NULL, UR_UNINITIALIZED};
