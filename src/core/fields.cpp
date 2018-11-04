/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/
// Tables are indexed by field ID
#include "fields.h"

char *ur_field_names_static[] = {
(char *)   "ACKCount",
(char *)   "ACKWaiting",
(char *)   "BYTE",
(char *)   "CANCount",
(char *)   "CMDCLASS",
(char *)   "GENRE",
(char *)   "GW_ID",
(char *)   "ID",
(char *)   "INDEX",
(char *)   "INSTANCE",
(char *)   "NAKCount",
(char *)   "OOFCount",
(char *)   "SOFCount",
(char *)   "TIME",
(char *)   "TYPE",
(char *)   "VALUE",
(char *)   "aclMtu",
(char *)   "aclPackets",
(char *)   "address",
(char *)   "average",
(char *)   "averageRequestRTT",
(char *)   "averageResponseRTT",
(char *)   "badChecksum",
(char *)   "badroutes",
(char *)   "broadcastReadCount",
(char *)   "broadcastWriteCount",
(char *)   "callbacks",
(char *)   "dropped",
(char *)   "err_value",
(char *)   "homeID",
(char *)   "lastRequestRTT",
(char *)   "lastResponseRTT",
(char *)   "moving_average",
(char *)   "moving_median",
(char *)   "moving_variance",
(char *)   "netBusy",
(char *)   "noACK",
(char *)   "nodeID",
(char *)   "nonDelivery",
(char *)   "notIdle",
(char *)   "profile_value",
(char *)   "quality",
(char *)   "readAborts",
(char *)   "readCount",
(char *)   "receiveDuplications",
(char *)   "receiveUnsolicited",
(char *)   "receivedCount",
(char *)   "retries",
(char *)   "routedBusy",
(char *)   "rxAcls",
(char *)   "rxBytes",
(char *)   "rxErrors",
(char *)   "rxEvents",
(char *)   "rxScos",
(char *)   "scoMtu",
(char *)   "scoPackets",
(char *)   "sentCount",
(char *)   "sentFailed",
(char *)   "txAcls",
(char *)   "txBytes",
(char *)   "txCmds",
(char *)   "txErrors",
(char *)   "txScos",
(char *)   "writeCount",
(char *)   "alert_desc",
(char *)   "cmd",
(char *)   "profile_key",
(char *)   "ur_key",
};
short ur_field_sizes_static[] = {
   8, /* ACKCount */
   8, /* ACKWaiting */
   8, /* BYTE */
   8, /* CANCount */
   8, /* CMDCLASS */
   8, /* GENRE */
   8, /* GW_ID */
   8, /* ID */
   8, /* INDEX */
   8, /* INSTANCE */
   8, /* NAKCount */
   8, /* OOFCount */
   8, /* SOFCount */
   8, /* TIME */
   8, /* TYPE */
   8, /* VALUE */
   8, /* aclMtu */
   8, /* aclPackets */
   8, /* address */
   8, /* average */
   8, /* averageRequestRTT */
   8, /* averageResponseRTT */
   8, /* badChecksum */
   8, /* badroutes */
   8, /* broadcastReadCount */
   8, /* broadcastWriteCount */
   8, /* callbacks */
   8, /* dropped */
   8, /* err_value */
   8, /* homeID */
   8, /* lastRequestRTT */
   8, /* lastResponseRTT */
   8, /* moving_average */
   8, /* moving_median */
   8, /* moving_variance */
   8, /* netBusy */
   8, /* noACK */
   8, /* nodeID */
   8, /* nonDelivery */
   8, /* notIdle */
   8, /* profile_value */
   8, /* quality */
   8, /* readAborts */
   8, /* readCount */
   8, /* receiveDuplications */
   8, /* receiveUnsolicited */
   8, /* receivedCount */
   8, /* retries */
   8, /* routedBusy */
   8, /* rxAcls */
   8, /* rxBytes */
   8, /* rxErrors */
   8, /* rxEvents */
   8, /* rxScos */
   8, /* scoMtu */
   8, /* scoPackets */
   8, /* sentCount */
   8, /* sentFailed */
   8, /* txAcls */
   8, /* txBytes */
   8, /* txCmds */
   8, /* txErrors */
   8, /* txScos */
   8, /* writeCount */
   -1, /* alert_desc */
   -1, /* cmd */
   -1, /* profile_key */
   -1, /* ur_key */
};
ur_field_type_t ur_field_types_static[] = {
   UR_TYPE_DOUBLE, /* ACKCount */
   UR_TYPE_DOUBLE, /* ACKWaiting */
   UR_TYPE_DOUBLE, /* BYTE */
   UR_TYPE_DOUBLE, /* CANCount */
   UR_TYPE_DOUBLE, /* CMDCLASS */
   UR_TYPE_DOUBLE, /* GENRE */
   UR_TYPE_UINT64, /* GW_ID */
   UR_TYPE_UINT64, /* ID */
   UR_TYPE_DOUBLE, /* INDEX */
   UR_TYPE_DOUBLE, /* INSTANCE */
   UR_TYPE_DOUBLE, /* NAKCount */
   UR_TYPE_DOUBLE, /* OOFCount */
   UR_TYPE_DOUBLE, /* SOFCount */
   UR_TYPE_DOUBLE, /* TIME */
   UR_TYPE_DOUBLE, /* TYPE */
   UR_TYPE_DOUBLE, /* VALUE */
   UR_TYPE_DOUBLE, /* aclMtu */
   UR_TYPE_DOUBLE, /* aclPackets */
   UR_TYPE_DOUBLE, /* address */
   UR_TYPE_DOUBLE, /* average */
   UR_TYPE_DOUBLE, /* averageRequestRTT */
   UR_TYPE_DOUBLE, /* averageResponseRTT */
   UR_TYPE_DOUBLE, /* badChecksum */
   UR_TYPE_DOUBLE, /* badroutes */
   UR_TYPE_DOUBLE, /* broadcastReadCount */
   UR_TYPE_DOUBLE, /* broadcastWriteCount */
   UR_TYPE_DOUBLE, /* callbacks */
   UR_TYPE_DOUBLE, /* dropped */
   UR_TYPE_DOUBLE, /* err_value */
   UR_TYPE_DOUBLE, /* homeID */
   UR_TYPE_DOUBLE, /* lastRequestRTT */
   UR_TYPE_DOUBLE, /* lastResponseRTT */
   UR_TYPE_DOUBLE, /* moving_average */
   UR_TYPE_DOUBLE, /* moving_median */
   UR_TYPE_DOUBLE, /* moving_variance */
   UR_TYPE_DOUBLE, /* netBusy */
   UR_TYPE_DOUBLE, /* noACK */
   UR_TYPE_DOUBLE, /* nodeID */
   UR_TYPE_DOUBLE, /* nonDelivery */
   UR_TYPE_DOUBLE, /* notIdle */
   UR_TYPE_DOUBLE, /* profile_value */
   UR_TYPE_DOUBLE, /* quality */
   UR_TYPE_DOUBLE, /* readAborts */
   UR_TYPE_DOUBLE, /* readCount */
   UR_TYPE_DOUBLE, /* receiveDuplications */
   UR_TYPE_DOUBLE, /* receiveUnsolicited */
   UR_TYPE_DOUBLE, /* receivedCount */
   UR_TYPE_DOUBLE, /* retries */
   UR_TYPE_DOUBLE, /* routedBusy */
   UR_TYPE_DOUBLE, /* rxAcls */
   UR_TYPE_DOUBLE, /* rxBytes */
   UR_TYPE_DOUBLE, /* rxErrors */
   UR_TYPE_DOUBLE, /* rxEvents */
   UR_TYPE_DOUBLE, /* rxScos */
   UR_TYPE_DOUBLE, /* scoMtu */
   UR_TYPE_DOUBLE, /* scoPackets */
   UR_TYPE_DOUBLE, /* sentCount */
   UR_TYPE_DOUBLE, /* sentFailed */
   UR_TYPE_DOUBLE, /* txAcls */
   UR_TYPE_DOUBLE, /* txBytes */
   UR_TYPE_DOUBLE, /* txCmds */
   UR_TYPE_DOUBLE, /* txErrors */
   UR_TYPE_DOUBLE, /* txScos */
   UR_TYPE_DOUBLE, /* writeCount */
   UR_TYPE_STRING, /* alert_desc */
   UR_TYPE_STRING, /* cmd */
   UR_TYPE_STRING, /* profile_key */
   UR_TYPE_STRING, /* ur_key */
};
ur_static_field_specs_t UR_FIELD_SPECS_STATIC = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, 68};
ur_field_specs_t ur_field_specs = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, 68, 68, 68, NULL, UR_UNINITIALIZED};
