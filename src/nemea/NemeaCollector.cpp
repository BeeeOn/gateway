/**
 * \file NemeaCollector.cpp
 * \brief Collect BeeeOn events and send them to the NEMEA
 * \author Dominik Soukup <soukudom@fit.cvut.cz>
 * \date 2018
**/

/*
 * Copyright (C) 2018 CESNET
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include <string>
#include <iostream>
#include <Poco/Timestamp.h>
#include "nemea/NemeaCollector.h"
#include "di/Injectable.h"
#include "model/DeviceID.h"
#include "model/SensorData.h"
#include "bluetooth/HciInfo.h"
#include "util/Occasionally.h"
#include <Poco/NumberFormatter.h>

#ifdef HAVE_ZWAVE
#include "zwave/ZWaveDriverEvent.h"
#include "zwave/ZWaveNodeEvent.h"
#endif

#ifdef HAVE_OPENZWAVE
#include "zwave/OZWNotificationEvent.h"
#endif

#ifdef HAVE_IQRF
#include "iqrf/IQRFEvent.h"
#endif

#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include "fields.h"

// BeeeOn initialization macros
BEEEON_OBJECT_BEGIN(BeeeOn, NemeaCollector)
BEEEON_OBJECT_CASTABLE(DistributorListener)       // Interface name for DependencyInjector
BEEEON_OBJECT_CASTABLE(ZWaveListener)             // Interface name for DependencyInjector
BEEEON_OBJECT_CASTABLE(HciListener) 	          // Interface name for DependencyInjector
BEEEON_OBJECT_CASTABLE(IQRFListener)              // Interface name for DependencyInjector
BEEEON_OBJECT_CASTABLE(CommandDispatcherListener)
BEEEON_OBJECT_PROPERTY("onExportInterface", &NemeaCollector::setOnExport)  // Member function for input param defined in the file factory.xml
BEEEON_OBJECT_PROPERTY("onHCIStatsInterface", &NemeaCollector::setOnHCIStats) // Member function for input param defined in the file factory.xml
#ifdef HAVE_ZWAVE
BEEEON_OBJECT_PROPERTY("onNodeStatsInterface", &NemeaCollector::setOnNodeStats) // Member function for input param defined in the file factory.xml
BEEEON_OBJECT_PROPERTY("onDriverStatsInterface", &NemeaCollector::setOnDriverStats) // Member function for input param defined in the file factory.xml
#endif
#ifdef HAVE_OPENZWAVE
BEEEON_OBJECT_PROPERTY("onNotificationInterface", &NemeaCollector::setOnNotification) // Member function for input param defined in the file factory.xml
#endif
#ifdef HAVE_IQRF
BEEEON_OBJECT_PROPERTY("onReceiveDPAInterface", &NemeaCollector::setOnReceiveDPA) // Member function for input param defined in the file factory.xml
#endif
BEEEON_OBJECT_PROPERTY("onDispatchInterface", &NemeaCollector::setOnDispatch) // Member function for input param defined in the file factory.xml
BEEEON_OBJECT_PROPERTY("exportGwID", &NemeaCollector::setExportGwID) // Member function for input param defined in the file factory.xml
BEEEON_OBJECT_END(BeeeOn, NemeaCollector)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

// Default constructor
EventMetaData::EventMetaData() : ctx(nullptr), utmpl(nullptr), udata(nullptr), uerr(nullptr), onEventInterface(""), ufields("") {}

// Default destructor
EventMetaData::~EventMetaData() {
    trap_ctx_finalize(&(ctx));
    ur_free_template(utmpl);
    ur_free_record(udata);
}

// Default contructor
NemeaCollector::NemeaCollector() = default;

// Default destructor
NemeaCollector::~NemeaCollector() = default;

// Initialize output interface parameters
void NemeaCollector::initInterface(EventMetaData &interfaceMetaInfo){
    // Interface initialization
    interfaceMetaInfo.ctx = trap_ctx_init3("stats-col", "sensor statistics collector", 0, 1,interfaceMetaInfo.onEventInterface.c_str(),NULL);
    if (interfaceMetaInfo.ctx == NULL){
        cerr << "ERROR in TRAP initialization " << trap_last_error_msg << endl;
        throw InvalidArgumentException("ERROR in TRAP initialization" + string(trap_last_error_msg));
    }

    if (trap_ctx_get_last_error(interfaceMetaInfo.ctx) != TRAP_E_OK){
        cerr << "ERROR in TRAP initialization: " << trap_ctx_get_last_error_msg(interfaceMetaInfo.ctx) << endl;
        throw InvalidArgumentException("ERROR in TRAP initialization: " + string(trap_ctx_get_last_error_msg(interfaceMetaInfo.ctx)));
   }

    // Interface control
    if (trap_ctx_ifcctl(interfaceMetaInfo.ctx, TRAPIFC_OUTPUT,0,TRAPCTL_SETTIMEOUT, TRAP_NO_WAIT) != TRAP_E_OK){
        cerr << "ERROR in output interface initialization" << endl;
        throw InvalidArgumentException("ERROR in output interface initialization");
    }

    // Unirec template definition
    interfaceMetaInfo.utmpl = ur_ctx_create_output_template(interfaceMetaInfo.ctx,0,interfaceMetaInfo.ufields.c_str(), &interfaceMetaInfo.uerr);
    if (interfaceMetaInfo.utmpl == NULL){
        cerr << "ERROR in UNIREC template definiton " << interfaceMetaInfo.uerr <<endl;
        throw InvalidArgumentException("ERROR in UNIREC template definiton " + string(interfaceMetaInfo.uerr));
    }

    // Create record
    interfaceMetaInfo.udata = ur_create_record(interfaceMetaInfo.utmpl, UR_MAX_SIZE);
    if ( interfaceMetaInfo.udata == NULL) {
        cerr << "ERROR: Unable to create unirec record" << endl;
        throw InvalidArgumentException("ERROR: Unable to create unirec record");
    }
}

void NemeaCollector::onExport(const SensorData &data) {
    // Catch current timestamp
    Poco::Timestamp now;
    int sensor_valueID_cnt = 0;
    ur_time_t timestamp = ur_time_from_sec_usec(now.epochTime(),now.epochMicroseconds());

    // Insert data into the unirec record
    for (auto const &module: data){
        ur_set(onExportMetaInfo.utmpl, onExportMetaInfo.udata, F_VALUE, module.value());
        ur_set(onExportMetaInfo.utmpl, onExportMetaInfo.udata, F_TIME, timestamp);
        ur_set(onExportMetaInfo.utmpl, onExportMetaInfo.udata, F_DEV_ADDR, data.deviceID()+sensor_valueID_cnt);

        // Change ID for multiple virtual sensors -> the are using the same id now
        sensor_valueID_cnt++;

        // Send out received data
        trap_ctx_send(onExportMetaInfo.ctx, 0, onExportMetaInfo.udata, ur_rec_size(onExportMetaInfo.utmpl, onExportMetaInfo.udata));
    }
}

#ifdef HAVE_ZWAVE
void NemeaCollector::onDriverStats(const ZWaveDriverEvent &event)
{
    // Catch current timestamp
    Poco::Timestamp now;
    ur_time_t timestamp = ur_time_from_sec_usec(now.epochTime(),now.epochMicroseconds());

    // Insert data into the unirec record
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_TIME, timestamp);
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_DEV_ADDR, exportGwID);
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_SOF_COUNT, event.SOFCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_ACK_WAITING, event.ACKWaiting());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_READ_ABORTS, event.readAborts());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_BAD_CHECKSUM, event.badChecksum());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_READ_COUNT, event.readCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_WRITE_COUNT, event.writeCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_CAN_COUNT, event.CANCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_NAK_COUNT, event.NAKCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_ACK_COUNT, event.ACKCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_OOF_COUNT, event.OOFCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_DROPPED, event.dropped());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_RETRIES, event.retries());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_CALLBACKS, event.callbacks());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_BAD_ROUTES, event.badroutes());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_NO_ACK, event.noACK());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_NET_BUSY, event.netBusy());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_NOT_IDLE, event.notIdle());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_NON_DELIVERY, event.nonDelivery());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_ROUTED_BUSY, event.routedBusy());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_BROADCAST_READ_COUNT, event.broadcastReadCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_BROADCAST_WRITE_COUNT, event.broadcastWriteCount());

    // Send out recived data
    trap_ctx_send(onDriverStatsMetaInfo.ctx, 0, onDriverStatsMetaInfo.udata, ur_rec_size(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata));
}

void NemeaCollector::onNodeStats(const ZWaveNodeEvent &event)
{
    // Catch current timestamp
    Poco::Timestamp now;
    ur_time_t timestamp = ur_time_from_sec_usec(now.epochTime(),now.epochMicroseconds());

    // Insert data into the unirec record
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_TIME, timestamp);
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_SENT_COUNT, event.sentCount());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_SENT_FAILED, event.sentFailed());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_RECEIVED_COUNT, event.receivedCount());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_RECEIVE_DUPLICATIONS, event.receiveDuplications());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_RECEIVE_UNSOLICITED, event.receiveUnsolicited());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_LAST_REQUEST_RTT, event.lastRequestRTT());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_LAST_RESPONSE_RTT, event.lastResponseRTT());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_AVERAGE_REQUEST_RTT, event.averageRequestRTT());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_AVERAGE_RESPONSE_RTT, event.sentCount());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_QUALITY, event.quality());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_DEV_ADDR, event.nodeID());

    // Send out received data
    trap_ctx_send(onNodeStatsMetaInfo.ctx, 0, onNodeStatsMetaInfo.udata, ur_rec_size(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata));
}
#else
void NemeaCollector::onDriverStats(const ZWaveDriverEvent &event) {}
void NemeaCollector::onNodeStats(const ZWaveNodeEvent &event) {}
#endif

#ifdef HAVE_HCI
void NemeaCollector::onHciStats(const HciInfo &event){

    // Catch current timestamp
    Poco::Timestamp now;
    ur_time_t timestamp = ur_time_from_sec_usec(now.epochTime(),now.epochMicroseconds());

    // Insert data into the unirec record
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_TIME, timestamp);
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_DEV_ADDR, exportGwID);
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_ADDRESS, event.address());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_ACL_PACKETS, event.aclPackets());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_ACL_MTU, event.aclMtu());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_SCO_MTU, event.scoMtu());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_SCO_PACKETS, event.scoPackets());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_RX_ERRORS, event.rxErrors());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_TX_ERRORS, event.txErrors());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_RX_EVENTS, event.rxEvents());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_TX_CMDS, event.txCmds());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_RX_ACLS, event.rxAcls());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_TX_ACLS, event.txAcls());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_RX_SCOS, event.rxScos());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_TX_SCOS, event.txScos());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_RX_BYTES, event.rxBytes());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_TX_BYTES, event.txBytes());

    // Send out recived data
    trap_ctx_send(onHCIStatsMetaInfo.ctx, 0, onHCIStatsMetaInfo.udata, ur_rec_size(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata));
}
#else
void NemeaCollector::onHciStats(const HciInfo &event){}
#endif

#ifdef HAVE_OPENZWAVE
void NemeaCollector::onNotification(const OZWNotificationEvent &event){

    // Catch current timestamp
    Poco::Timestamp now;
    ur_time_t timestamp = ur_time_from_sec_usec(now.epochTime(),now.epochMicroseconds());

    // Insert data into the unirec record
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_TIME, timestamp);
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_DEV_ADDR, exportGwID);
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_HOME_ID, event.homeID());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_NODE_ID, event.nodeID());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_GENRE, event.valueID().GetGenre());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_CMDCLASS, event.valueID().GetCommandClassId());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_INSTANCE, event.valueID().GetInstance());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_INDEX, event.valueID().GetIndex());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_TYPE, event.valueID().GetType());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_BYTE, event.byte());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_EVENT_TYPE, event.type());

    // Send out recived data
    trap_ctx_send(onNotificationMetaInfo.ctx, 0, onNotificationMetaInfo.udata, ur_rec_size(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata));
}
#else
void NemeaCollector::onNotification(const OZWNotificationEvent &event){}
#endif

#ifdef HAVE_IQRF
void NemeaCollector::onReceiveDPA(const IQRFEvent &event)
{
    // Catch current timestamp
    Poco::Timestamp now;
    ur_time_t timestamp = ur_time_from_sec_usec(now.epochTime(),now.epochMicroseconds());

    // Insert data into the unirec record
    ur_set(onReceiveDPAMetaInfo.utmpl, onReceiveDPAMetaInfo.udata, F_TIME, timestamp);
    ur_set(onReceiveDPAMetaInfo.utmpl, onReceiveDPAMetaInfo.udata, F_DEV_ADDR, event.networkAddress());
    ur_set(onReceiveDPAMetaInfo.utmpl, onReceiveDPAMetaInfo.udata, F_TYPE, event.direction());
    ur_set(onReceiveDPAMetaInfo.utmpl, onReceiveDPAMetaInfo.udata, F_MESSAGE_TYPE, event.commandCode());
    ur_set(onReceiveDPAMetaInfo.utmpl, onReceiveDPAMetaInfo.udata, F_SIZE, event.size());
    ur_set_var(onReceiveDPAMetaInfo.utmpl, onReceiveDPAMetaInfo.udata, F_PAYLOAD, reinterpret_cast<char*>(event.payload().data()), event.payload().size());
    ur_set(onReceiveDPAMetaInfo.utmpl, onReceiveDPAMetaInfo.udata, F_INDEX, event.peripheralNumber());

    // Send out received data immediately
    trap_ctx_send(onReceiveDPAMetaInfo.ctx, 0, onReceiveDPAMetaInfo.udata, ur_rec_size(onReceiveDPAMetaInfo.utmpl, onReceiveDPAMetaInfo.udata));
    trap_ctx_send_flush(onReceiveDPAMetaInfo.ctx,0);
}
#else
void NemeaCollector::onReceiveDPA(const IQRFEvent &event){}
#endif

void NemeaCollector::onDispatch(const Command::Ptr cmd){
    // Catch current timestamp
    Poco::Timestamp now;
    ur_time_t timestamp = ur_time_from_sec_usec(now.epochTime(),now.epochMicroseconds());

    // Insert data into the unirec record
    ur_set(onDispatchMetaInfo.utmpl, onDispatchMetaInfo.udata, F_TIME, timestamp);
    ur_set(onDispatchMetaInfo.utmpl, onDispatchMetaInfo.udata, F_DEV_ADDR, exportGwID);
    ur_set_string(onDispatchMetaInfo.utmpl, onDispatchMetaInfo.udata, F_CMD, cmd->toString().c_str());

    // Send out recived data
    trap_ctx_send(onDispatchMetaInfo.ctx, 0, onDispatchMetaInfo.udata, ur_rec_size(onDispatchMetaInfo.utmpl, onDispatchMetaInfo.udata));
}

void NemeaCollector::setOnExport(const string& interface) {
    onExportMetaInfo.onEventInterface = interface;
    onExportMetaInfo.ufields = "TIME,DEV_ADDR,VALUE";
    initInterface(onExportMetaInfo);
}

#ifdef HAVE_HCI
void NemeaCollector::setOnHCIStats(const string& interface) {
    onHCIStatsMetaInfo.onEventInterface = interface;
    onHCIStatsMetaInfo.ufields = "TIME,DEV_ADDR,ADDRESS,ACL_PACKETS,SCO_MTU,SCO_PACKETS,RX_ERRORS,TX_ERRORS,RX_EVENTS, TX_CMDS,TX_ACLS,TX_ACLS,TX_SCOS,TX_SCOS,TX_BYTES,TX_BYTES";
    initInterface(onHCIStatsMetaInfo);
}
#else
void NemeaCollector::setOnHCIStats(const string& interface) {}
#endif


#ifdef HAVE_ZWAVE
void NemeaCollector::setOnNodeStats(const string& interface) {
    onNodeStatsMetaInfo.onEventInterface = interface;
    onNodeStatsMetaInfo.ufields = "TIME,DEV_ADDR,SENT_COUNT,SENT_FAILED,RECEIVED_COUNT,RECEIVE_DUPLICATIONS,RECEIVE_UNSOLICITED,LAST_REQUEST_RTT,LAST_RESPONSE_RTT,AVERAGE_REQUEST_RTT,AVERAGE_RESPONSE_RTT,QUALITY";
    initInterface(onNodeStatsMetaInfo);
}

void NemeaCollector::setOnDriverStats(const string& interface) {
    onDriverStatsMetaInfo.onEventInterface = interface;
    onDriverStatsMetaInfo.ufields = "TIME,DEV_ADDR,SOF_COUNT,ACK_WAITING,READ_ABORTS,BAD_CHECKSUM,READ_COUNT,WRITE_COUNT,CAN_COUNT,NAK_COUNT,ACK_COUNT,OOF_COUNT,DROPPED,RETRIES,CALLBACKS,BAD_ROUTES,NO_ACK,NET_BUSY,NOT_IDLE,NON_DELIVERY,ROUTED_BUSY,BROADCAST_READ_COUNT,BROADCAST_WRITE_COUNT";
    initInterface(onDriverStatsMetaInfo);
}
#else
void NemeaCollector::setOnNodeStats(const string& interface) {}
void NemeaCollector::setOnDriverStats(const string& interface) {}
#endif

#ifdef HAVE_OPENZWAVE
void NemeaCollector::setOnNotification(const string& interface) {
    onNotificationMetaInfo.onEventInterface = interface;
    onNotificationMetaInfo.ufields = "TIME,DEV_ADDR,HOME_ID,NODE_ID,GENRE,CMDCLASS,INSTANCE,INDEX,TYPE,BYTE,EVENT_TYPE";
    initInterface(onNotificationMetaInfo);
}
#else
void NemeaCollector::setOnNotification(const string& interface) {}
#endif

#ifdef HAVE_IQRF
void NemeaCollector::setOnReceiveDPA(const string& interface)
{
    onReceiveDPAMetaInfo.onEventInterface = interface;
    onReceiveDPAMetaInfo.ufields = "TIME,DEV_ADDR,TYPE,MESSAGE_TYPE,SIZE,PAYLOAD,INDEX";
    initInterface(onReceiveDPAMetaInfo);
}
#else
void NemeaCollector::setOnReceiveDPA(const string& interface){}
#endif

void NemeaCollector::setOnDispatch(const string& interface) {
    onDispatchMetaInfo.onEventInterface = interface;
    onDispatchMetaInfo.ufields = "TIME,DEV_ADDR,CMD";
    initInterface(onDispatchMetaInfo);
}

void NemeaCollector::setExportGwID (const string &exportGwID){
    try{
        this->exportGwID = stoi(exportGwID);
    } catch (const std::exception& e){
        throw InvalidArgumentException("ERROR during conversion exportGwID to type 'int'");
    }
}
