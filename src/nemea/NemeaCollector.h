/**
 * \file NemeaCollector.h
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

#include "core/AbstractCollector.h"
#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include <string>

using namespace std;

namespace BeeeOn {
    /*
    * Subclass for NemeaCollector
    * Handle necessary information of libtrap and unirec for every event
    */
    class EventMetaData{
    public:
        // Default constructor
        EventMetaData();
        // Defaut destructor
        ~EventMetaData();

        trap_ctx_t *ctx;         // Trap context
        ur_template_t *utmpl;    // Unirec template
        void *udata;             // Unirec data
        char *uerr;              // Unirec error
        string onEventInterface; // Name of trap output interface
        string ufields;          // Field names in unirec message
    };


    /*
    * NemeaCollector class for collecting data for statistics and analysis purposes
    */
    class NemeaCollector : public AbstractCollector {
    public:
        // Default constructor
	NemeaCollector();
        // Default destructor
	~NemeaCollector();

        /*
        * Events inherited from AbstractCollector
        */
        /**
        * Process data values from sensors
        * \param[in] data BeeeOn class for sensor data
        */
        void onExport (const SensorData &data) override;
        /**
        * Process data from Z-Wave interface
        * \param[in] even BeeeOn class for Z-Wave interface statistics
        */
        void onDriverStats (const ZWaveDriverEvent &event) override;
        /**
        * Process data from every Z-Wave node
        * \param[in] event BeeeOn class for Z-Wave node statistics
        */
        void onNodeStats (const ZWaveNodeEvent &event) override;
        /**
        * Process data from BLE interface
        * \param[in] info BeeeOn class for BLE interface statistics
        */
        void onHciStats (const HciInfo &info) override;

        /**
        * Process data from OZW notification event
        * \param[in] info BeeeOn class for OZW notification statistics
        */
    	void onNotification(const OZWNotificationEvent &event) override;

        /**
        * Process data from IQRF interface
        * \param[in] info BeeeOn class for IQRF statistics
        */
        void onReceiveDPA(const BeeeOn::IQRFEvent&) override;

        /**
        * Process data from BeeeOn control commands
        * \param[in] info BeeeOn class for control commands
        */
        void onDispatch(const Command::Ptr cmd) override;

        /*
        * Setters for input data specified in the file factory.xml
        * Each event has its own setter
        * Each setter receives parameter value as input param
        */
        void setOnExport (const string &interface);
        void setOnHCIStats (const string &interface);
        void setOnNodeStats (const string &interface);
        void setOnDriverStats (const string &interface);
        void setOnNotification (const string &interface);
        void setOnReceiveDPA(const string &interface);
        void setOnDispatch (const string &interface);
        void setExportGwID (const string &gwID);

        /*
        * Responsible for output interface initialization
        * \param[in] interfaceMetaInfo Instace of class EventMetaData which handle meta information for one event
        */
        void initInterface(EventMetaData& interfaceMetaInfo);

    private:
        // EventMetaData instance for each event
        EventMetaData onExportMetaInfo;
        EventMetaData onHCIStatsMetaInfo;
        EventMetaData onNodeStatsMetaInfo;
        EventMetaData onDriverStatsMetaInfo;
        EventMetaData onNotificationMetaInfo;
        EventMetaData onReceiveDPAMetaInfo;
        EventMetaData onDispatchMetaInfo;
        int exportGwID; // ID to identify data from this gateway
    };
}
