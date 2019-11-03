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

#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include "fields.h"

// BeeeOn initialization macros
BEEEON_OBJECT_BEGIN(BeeeOn, NemeaCollector)
BEEEON_OBJECT_CASTABLE(DistributorListener)       // Interface name for DependencyInjector
BEEEON_OBJECT_CASTABLE(ZWaveListener)             // Interface name for DependencyInjector
BEEEON_OBJECT_CASTABLE(HciListener) 	          // Interface name for DependencyInjector
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
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_SOFCount, event.SOFCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_ACKWaiting, event.ACKWaiting());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_readAborts, event.readAborts());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_badChecksum, event.badChecksum());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_readCount, event.readCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_writeCount, event.writeCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_CANCount, event.CANCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_NAKCount, event.NAKCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_ACKCount, event.ACKCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_OOFCount, event.OOFCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_dropped, event.dropped());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_retries, event.retries());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_callbacks, event.callbacks());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_badroutes, event.badroutes());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_noACK, event.noACK());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_netBusy, event.netBusy());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_notIdle, event.notIdle());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_nonDelivery, event.nonDelivery());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_routedBusy, event.routedBusy());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_broadcastReadCount, event.broadcastReadCount());
    ur_set(onDriverStatsMetaInfo.utmpl, onDriverStatsMetaInfo.udata, F_broadcastWriteCount, event.broadcastWriteCount());

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
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_sentCount, event.sentCount());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_sentFailed, event.sentFailed());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_receivedCount, event.receivedCount());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_receiveDuplications, event.receiveDuplications());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_receiveUnsolicited, event.receiveUnsolicited());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_lastRequestRTT, event.lastRequestRTT());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_lastResponseRTT, event.lastResponseRTT());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_averageRequestRTT, event.averageRequestRTT());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_averageResponseRTT, event.sentCount());
    ur_set(onNodeStatsMetaInfo.utmpl, onNodeStatsMetaInfo.udata, F_quality, event.quality());
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
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_address, event.address());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_aclPackets, event.aclPackets());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_aclMtu, event.aclMtu());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_scoMtu, event.scoMtu());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_scoPackets, event.scoPackets());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_rxErrors, event.rxErrors());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_txErrors, event.txErrors());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_rxEvents, event.rxEvents());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_txCmds, event.txCmds());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_rxAcls, event.rxAcls());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_txAcls, event.txAcls());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_rxScos, event.rxScos());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_txScos, event.txScos());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_rxBytes, event.rxBytes());
    ur_set(onHCIStatsMetaInfo.utmpl, onHCIStatsMetaInfo.udata, F_txBytes, event.txBytes());

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
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_homeID, event.homeID());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_nodeID, event.nodeID());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_GENRE, event.valueID().GetGenre());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_CMDCLASS, event.valueID().GetCommandClassId());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_INSTANCE, event.valueID().GetInstance());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_INDEX, event.valueID().GetIndex());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_TYPE, event.valueID().GetType());
    ur_set(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata, F_BYTE, event.byte());

    // Send out recived data
    trap_ctx_send(onNotificationMetaInfo.ctx, 0, onNotificationMetaInfo.udata, ur_rec_size(onNotificationMetaInfo.utmpl, onNotificationMetaInfo.udata));
}
#else
void NemeaCollector::onNotification(const OZWNotificationEvent &event){}
#endif

void NemeaCollector::onDispatch(const Command::Ptr cmd){
    // Catch current timestamp
    Poco::Timestamp now;
    ur_time_t timestamp = ur_time_from_sec_usec(now.epochTime(),now.epochMicroseconds());

    // Insert data into the unirec record
    ur_set(onDispatchMetaInfo.utmpl, onDispatchMetaInfo.udata, F_TIME, timestamp);
    ur_set(onDispatchMetaInfo.utmpl, onDispatchMetaInfo.udata, F_DEV_ADDR, exportGwID);
    ur_set_string(onDispatchMetaInfo.utmpl, onDispatchMetaInfo.udata, F_cmd, cmd->toString().c_str());

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
    onHCIStatsMetaInfo.ufields = "TIME,DEV_ADDR,address,aclPackets,scoMtu,scoPackets,rxErrors,txErrors,rxEvents, txCmds,rxAcls,txAcls,rxScos,txScos,rxBytes,txBytes";
    initInterface(onHCIStatsMetaInfo);
}
#else
void NemeaCollector::setOnHCIStats(const string& interface) {}
#endif


#ifdef HAVE_ZWAVE
void NemeaCollector::setOnNodeStats(const string& interface) {
    onNodeStatsMetaInfo.onEventInterface = interface;
    onNodeStatsMetaInfo.ufields = "TIME,DEV_ADDR,sentCount,sentFailed,receivedCount,receiveDuplications,receiveUnsolicited,lastRequestRTT,lastResponseRTT,averageRequestRTT,averageResponseRTT,quality";
    initInterface(onNodeStatsMetaInfo);
}

void NemeaCollector::setOnDriverStats(const string& interface) {
    onDriverStatsMetaInfo.onEventInterface = interface;
    onDriverStatsMetaInfo.ufields = "TIME,DEV_ADDR,SOFCount,ACKWaiting,readAborts,badChecksum,readCount,writeCount,CANCount,NAKCount,ACKCount,OOFCount,dropped,retries,callbacks,badroutes,noACK,netBusy,notIdle,nonDelivery,routedBusy,broadcastReadCount,broadcastWriteCount";
    initInterface(onDriverStatsMetaInfo);
}
#else
void NemeaCollector::setOnNodeStats(const string& interface) {}
void NemeaCollector::setOnDriverStats(const string& interface) {}
#endif

#ifdef HAVE_OPENZWAVE
void NemeaCollector::setOnNotification(const string& interface) {
    onNotificationMetaInfo.onEventInterface = interface;
    onNotificationMetaInfo.ufields = "TIME,DEV_ADDR,homeID,nodeID,GENRE,CMDCLASS,INSTANCE,INDEX,TYPE,BYTE";
    initInterface(onNotificationMetaInfo);
}
#else
void NemeaCollector::setOnNotification(const string& interface) {}
#endif

void NemeaCollector::setOnDispatch(const string& interface) {
    onDispatchMetaInfo.onEventInterface = interface;
    onDispatchMetaInfo.ufields = "TIME,DEV_ADDR,cmd";
    initInterface(onDispatchMetaInfo);
}

void NemeaCollector::setExportGwID (const string &exportGwID){
    try{
        this->exportGwID = stoi(exportGwID);
    } catch (const std::exception& e){
        throw InvalidArgumentException("ERROR during conversion exportGwID to type 'int'");
    }
}
