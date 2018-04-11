cmake_minimum_required (VERSION 2.8.11)
project (gateway-core CXX C)

find_library (POCO_FOUNDATION PocoFoundation)
find_library (POCO_SSL PocoNetSSL)
find_library (POCO_CRYPTO PocoCrypto)
find_library (POCO_UTIL PocoUtil)
find_library (POCO_NET PocoNet)
find_library (POCO_JSON PocoJSON)
find_library (POCO_XML PocoXML)
find_library (PTHREAD pthread)
find_package (PkgConfig REQUIRED)
pkg_search_module (GLIB REQUIRED glib-2.0)
pkg_search_module (GIO_UNIX REQUIRED gio-unix-2.0)
pkg_search_module (BLUEZ REQUIRED bluez)
find_program (GDBUS_CODEGEN NAMES gdbus-codegen)

if(GLIB_FOUND AND GIO_UNIX_FOUND AND GDBUS_CODEGEN AND BLUEZ_FOUND)
	include_directories(${GIO_UNIX_INCLUDE_DIRS})
	include_directories(${GLIB_INCLUDE_DIRS})
	set(HAS_DBUS_BLUEZ YES)
endif()

if(ENABLE_UDEV)
	find_library (UDEV udev)
endif()

if(ENABLE_BLUETOOTH)
	find_library (BLUETOOTH bluetooth)

	if(HAS_DBUS_BLUEZ)
		file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/bluetooth)
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/bluetooth/org-bluez-adapter1.c
			COMMAND ${GDBUS_CODEGEN} --interface-prefix org.bluez.Adapter1. --generate-c-code ${PROJECT_BINARY_DIR}/bluetooth/org-bluez-adapter1 ${PROJECT_SOURCE_DIR}/bluetooth/dbus/org.bluez.Adapter1.xml
			DEPENDS ${PROJECT_SOURCE_DIR}/bluetooth/dbus/org.bluez.Adapter1.xml
			COMMENT "Generate DBus 'org.bluez.Adapter1.xml'"
		)
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/bluetooth/org-bluez-device1.c
			COMMAND ${GDBUS_CODEGEN} --interface-prefix org.bluez.Device1. --generate-c-code ${PROJECT_BINARY_DIR}/bluetooth/org-bluez-device1 ${PROJECT_SOURCE_DIR}/bluetooth/dbus/org.bluez.Device1.xml
			DEPENDS ${PROJECT_SOURCE_DIR}/bluetooth/dbus/org.bluez.Device1.xml
			COMMENT "Generate DBus 'org.bluez.Device1.xml'"
		)
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/bluetooth/org-bluez-gattcharacteristic1.c
			COMMAND ${GDBUS_CODEGEN} --interface-prefix org.bluez.GattCharacteristic1. --generate-c-code ${PROJECT_BINARY_DIR}/bluetooth/org-bluez-gattcharacteristic1 ${PROJECT_SOURCE_DIR}/bluetooth/dbus/org.bluez.GattCharacteristic1.xml
			DEPENDS ${PROJECT_SOURCE_DIR}/bluetooth/dbus/org.bluez.GattCharacteristic1.xml
			COMMENT "Generate DBus 'org.bluez.GattCharacteristic1.xml'"
		)
		include_directories(${PROJECT_BINARY_DIR})
	endif()
endif()

if(ENABLE_ZWAVE)
	find_package(OpenZWave REQUIRED)
	include_directories(${OPENZWAVE_INCLUDE_DIR})
endif()

if(ENABLE_FITP)
	find_library (FITP_LIB fitp)
endif()

if (ENABLE_MOSQUITTO_CLIENT)
	find_library (MOSQUITTO_CPP mosquittopp)
endif()

if(ENABLE_UCLIBCXX_FIXES)

add_definitions(-DNO_std_to_string)
set(CMAKE_CXX_FLAGS "-I${PROJECT_SOURCE_DIR}/../base/src/uclibc++ ${CMAKE_CXX_FLAGS}")

endif()

file(GLOB SOURCES
	${PROJECT_SOURCE_DIR}/bluetooth/BluetoothListener.cpp
	${PROJECT_SOURCE_DIR}/commands/DeviceAcceptCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/DeviceSetValueCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/DeviceUnpairCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/DeviceUnpairResult.cpp
	${PROJECT_SOURCE_DIR}/commands/GatewayListenCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/NewDeviceCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/ServerDeviceListCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/ServerDeviceListResult.cpp
	${PROJECT_SOURCE_DIR}/commands/ServerLastValueCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/ServerLastValueResult.cpp
	${PROJECT_SOURCE_DIR}/core/AbstractDistributor.cpp
	${PROJECT_SOURCE_DIR}/core/AbstractCollector.cpp
	${PROJECT_SOURCE_DIR}/core/AbstractSeeker.cpp
	${PROJECT_SOURCE_DIR}/core/Answer.cpp
	${PROJECT_SOURCE_DIR}/core/AnswerQueue.cpp
	${PROJECT_SOURCE_DIR}/core/AsyncCommandDispatcher.cpp
	${PROJECT_SOURCE_DIR}/core/Command.cpp
	${PROJECT_SOURCE_DIR}/core/CommandDispatcher.cpp
	${PROJECT_SOURCE_DIR}/core/CommandDispatcherListener.cpp
	${PROJECT_SOURCE_DIR}/core/CommandHandler.cpp
	${PROJECT_SOURCE_DIR}/core/CommandSender.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceCache.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceManager.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceStatusFetcher.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceStatusHandler.cpp
	${PROJECT_SOURCE_DIR}/core/Distributor.cpp
	${PROJECT_SOURCE_DIR}/core/DistributorListener.cpp
	${PROJECT_SOURCE_DIR}/core/DongleDeviceManager.cpp
	${PROJECT_SOURCE_DIR}/core/Exporter.cpp
	${PROJECT_SOURCE_DIR}/core/FilesystemDeviceCache.cpp
	${PROJECT_SOURCE_DIR}/core/GatewayInfo.cpp
	${PROJECT_SOURCE_DIR}/core/LoggingCollector.cpp
	${PROJECT_SOURCE_DIR}/core/MemoryDeviceCache.cpp
	${PROJECT_SOURCE_DIR}/core/PrefixCommand.cpp
	${PROJECT_SOURCE_DIR}/core/Result.cpp
	${PROJECT_SOURCE_DIR}/core/QueuingExporter.cpp
	${PROJECT_SOURCE_DIR}/credentials/Credentials.cpp
	${PROJECT_SOURCE_DIR}/credentials/CredentialsStorage.cpp
	${PROJECT_SOURCE_DIR}/credentials/CredentialsTool.cpp
	${PROJECT_SOURCE_DIR}/credentials/FileCredentialsStorage.cpp
	${PROJECT_SOURCE_DIR}/credentials/PasswordCredentials.cpp
	${PROJECT_SOURCE_DIR}/credentials/PinCredentials.cpp
	${PROJECT_SOURCE_DIR}/exporters/InMemoryQueuingStrategy.cpp
	${PROJECT_SOURCE_DIR}/exporters/JournalQueuingStrategy.cpp
	${PROJECT_SOURCE_DIR}/exporters/NamedPipeExporter.cpp
	${PROJECT_SOURCE_DIR}/exporters/QueuingStrategy.cpp
	${PROJECT_SOURCE_DIR}/exporters/RecoverableJournalQueuingStrategy.cpp
	${PROJECT_SOURCE_DIR}/hotplug/AbstractHotplugMonitor.cpp
	${PROJECT_SOURCE_DIR}/hotplug/HotplugEvent.cpp
	${PROJECT_SOURCE_DIR}/hotplug/HotplugListener.cpp
	${PROJECT_SOURCE_DIR}/hotplug/PipeHotplugMonitor.cpp
	${PROJECT_SOURCE_DIR}/net/AbstractHTTPScanner.cpp
	${PROJECT_SOURCE_DIR}/net/MqttMessage.cpp
	${PROJECT_SOURCE_DIR}/net/SOAPMessage.cpp
	${PROJECT_SOURCE_DIR}/net/UPnP.cpp
	${PROJECT_SOURCE_DIR}/net/VPTHTTPScanner.cpp
	${PROJECT_SOURCE_DIR}/philips/PhilipsHueListener.cpp
	${PROJECT_SOURCE_DIR}/server/GWContextPoll.cpp
	${PROJECT_SOURCE_DIR}/server/GWMessageContext.cpp
	${PROJECT_SOURCE_DIR}/server/GWSOutputQueue.cpp
	${PROJECT_SOURCE_DIR}/server/GWServerConnector.cpp
	${PROJECT_SOURCE_DIR}/server/ServerAnswer.cpp
	${PROJECT_SOURCE_DIR}/util/ColorBrightness.cpp
	${PROJECT_SOURCE_DIR}/util/CSVSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/DataReader.cpp
	${PROJECT_SOURCE_DIR}/util/DataWriter.cpp
	${PROJECT_SOURCE_DIR}/util/Journal.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataParser.cpp
	${PROJECT_SOURCE_DIR}/util/NullSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/SensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/SensorDataParser.cpp
	${PROJECT_SOURCE_DIR}/zwave/ZWaveListener.cpp
)

if(ENABLE_UDEV)
	file(GLOB UDEV_SOURCES ${PROJECT_SOURCE_DIR}/hotplug/UDevMonitor.cpp)
	list(APPEND SOURCES ${UDEV_SOURCES})
else()
	message(STATUS "udev support is disabled")
endif()

if(ENABLE_BELKIN_WEMO)
	file(GLOB BELKIN_SOURCES
		${PROJECT_SOURCE_DIR}/belkin/BelkinWemoBulb.cpp
		${PROJECT_SOURCE_DIR}/belkin/BelkinWemoDevice.cpp
		${PROJECT_SOURCE_DIR}/belkin/BelkinWemoDeviceManager.cpp
		${PROJECT_SOURCE_DIR}/belkin/BelkinWemoDimmer.cpp
		${PROJECT_SOURCE_DIR}/belkin/BelkinWemoLink.cpp
		${PROJECT_SOURCE_DIR}/belkin/BelkinWemoStandaloneDevice.cpp
		${PROJECT_SOURCE_DIR}/belkin/BelkinWemoSwitch.cpp
	)
	list(APPEND SOURCES ${BELKIN_SOURCES})
else()
	message(STATUS "Belkin Wemo support is disabled")
endif()

if(ENABLE_BLUETOOTH)
	file(GLOB BLUETOOTH_SOURCES
		${PROJECT_SOURCE_DIR}/bluetooth/BluezHciInterface.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/DBusHciConnection.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/DBusHciInterface.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/HciConnection.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/HciInfo.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/HciInterface.cpp
	)
	list(APPEND SOURCES ${BLUETOOTH_SOURCES})
	if(HAS_DBUS_BLUEZ)
		list(APPEND SOURCES
			${PROJECT_BINARY_DIR}/bluetooth/org-bluez-adapter1.c
			${PROJECT_BINARY_DIR}/bluetooth/org-bluez-device1.c
			${PROJECT_BINARY_DIR}/bluetooth/org-bluez-gattcharacteristic1.c
		)
		set_source_files_properties(
			${PROJECT_BINARY_DIR}/bluetooth/org-bluez-adapter1.c
			${PROJECT_BINARY_DIR}/bluetooth/org-bluez-device1.c
			${PROJECT_BINARY_DIR}/bluetooth/org-bluez-gattcharacteristic1.c
			PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-pedantic"
		)
	endif()
	add_definitions(-DHAVE_HCI=1)
else()
	message(STATUS "Bluetooth support is disabled")
endif()

if(ENABLE_BLUETOOTH AND ENABLE_BLUETOOTH_AVAILABILITY)
	file(GLOB BLUETOOTH_AVAILABILITY_SOURCES
		${PROJECT_SOURCE_DIR}/bluetooth/BluetoothAvailabilityManager.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/BluetoothDevice.cpp
	)
	list(APPEND SOURCES ${BLUETOOTH_AVAILABILITY_SOURCES})
else()
	message(STATUS "Bluetooth Avaibility support is disabled")
endif()

if(ENABLE_JABLOTRON)
	file(GLOB JABLOTRON_SOURCES
		${PROJECT_SOURCE_DIR}/jablotron/JablotronDevice.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceAC88.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceJA82SH.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceJA83P.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceJA85ST.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceManager.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceOpenClose.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceRC86K.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceTP82N.cpp
	)
	list(APPEND SOURCES ${JABLOTRON_SOURCES})
else()
	message(STATUS "Jablotron support is disabled")
endif()

if(ENABLE_PHILIPS_HUE)
	file(GLOB PHILIPS_SOURCES
		${PROJECT_SOURCE_DIR}/philips/PhilipsHueBulb.cpp
		${PROJECT_SOURCE_DIR}/philips/PhilipsHueBulbInfo.cpp
		${PROJECT_SOURCE_DIR}/philips/PhilipsHueBridge.cpp
		${PROJECT_SOURCE_DIR}/philips/PhilipsHueBridgeInfo.cpp
		${PROJECT_SOURCE_DIR}/philips/PhilipsHueDeviceManager.cpp
		${PROJECT_SOURCE_DIR}/philips/PhilipsHueDimmableBulb.cpp
	)
	list(APPEND SOURCES ${PHILIPS_SOURCES})
	add_definitions(-DHAVE_PHILIPS_HUE=1)
else()
	message(STATUS "Philips Hue support is disabled")
endif()

if(ENABLE_VIRTUAL_DEVICES)
	file(GLOB VIRTUAL_DEVICES_SOURCES
		${PROJECT_SOURCE_DIR}/vdev/VirtualModule.cpp
		${PROJECT_SOURCE_DIR}/vdev/VirtualDevice.cpp
		${PROJECT_SOURCE_DIR}/vdev/VirtualDeviceManager.cpp
	)
	list(APPEND SOURCES ${VIRTUAL_DEVICES_SOURCES})
else()
	message(STATUS "Virtual Devices support is disabled")
endif()

if(ENABLE_VPT)
	file(GLOB VPT_SOURCES
		${PROJECT_SOURCE_DIR}/vpt/VPTBoilerModuleType.cpp
		${PROJECT_SOURCE_DIR}/vpt/VPTDevice.cpp
		${PROJECT_SOURCE_DIR}/vpt/VPTDeviceManager.cpp
		${PROJECT_SOURCE_DIR}/vpt/VPTValuesParser.cpp
		${PROJECT_SOURCE_DIR}/vpt/VPTZoneModuleType.cpp
	)
	list(APPEND SOURCES ${VPT_SOURCES})
else()
	message(STATUS "VPT support is disabled")
endif()

if(ENABLE_ZWAVE)
	file(GLOB ZWAVE_SOURCES
		${PROJECT_SOURCE_DIR}/zwave/DefaultZWaveDeviceInfo.cpp
		${PROJECT_SOURCE_DIR}/zwave/DefaultZWaveDeviceInfoRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/GenericZWaveDeviceInfoRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/VendorZWaveDeviceInfoRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveClassRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveDeviceInfo.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveDeviceManager.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveDriver.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveDriverEvent.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveNodeEvent.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveNodeInfo.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveNotificationEvent.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWavePocoLoggerAdapter.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveUtil.cpp
	)
	list(APPEND SOURCES ${ZWAVE_SOURCES})
	add_definitions(-DHAVE_ZWAVE=1)
else()
	message(STATUS "Z-Wave support is disabled")
endif()

if((ENABLE_MOSQUITTO_EXPORTER) AND (ENABLE_MOSQUITTO_CLIENT))
	file(GLOB MOSQUITTO_EXPORTER_SOURCES
		${PROJECT_SOURCE_DIR}/exporters/MosquittoExporter.cpp
	)
	list(APPEND SOURCES ${MOSQUITTO_EXPORTER_SOURCES})
else()
	message(STATUS "Mosquitto Exporting support is disabled")
endif()

if(ENABLE_BASIC_DISTRIBUTOR)
	file(GLOB BASIC_DISTRIBUTOR_SOURCES
		${PROJECT_SOURCE_DIR}/core/BasicDistributor.cpp
	)
	list(APPEND SOURCES ${BASIC_DISTRIBUTOR_SOURCES})
else()
	message(STATUS "Basic Distributor support is disabled")
endif()

if(ENABLE_QUEUING_DISTRIBUTOR)
	file(GLOB QUEUING_DISTRIBUTOR_SOURCES
		${PROJECT_SOURCE_DIR}/core/ExporterQueue.cpp
		${PROJECT_SOURCE_DIR}/core/QueuingDistributor.cpp
	)
	list(APPEND SOURCES ${QUEUING_DISTRIBUTOR_SOURCES})
else()
	message(STATUS "Queuing Distributor support is disabled")
endif()

if (ENABLE_TESTING_CENTER)
	file(GLOB TESTING_CENTER_SOURCES
		${PROJECT_SOURCE_DIR}/core/TestingCenter.cpp
	)
	list(APPEND SOURCES ${TESTING_CENTER_SOURCES})
else()
	message(STATUS "Testing Center support is disabled")
endif()

if (ENABLE_PRESSURE_SENSOR)
	file(GLOB PRESSURE_SENSOR_SOURCES
		${PROJECT_SOURCE_DIR}/psdev/PressureSensorManager.cpp
	)
	list(APPEND SOURCES ${PRESSURE_SENSOR_SOURCES})
else()
	message(STATUS "Internal air pressure sensor support is disabled")
endif()

if(ENABLE_FITP)
	file(GLOB FITP_SOURCES
		${PROJECT_SOURCE_DIR}/fitp/FitpDevice.cpp
		${PROJECT_SOURCE_DIR}/fitp/FitpDeviceManager.cpp
	)
	list(APPEND SOURCES ${FITP_SOURCES})
else()
	message(STATUS "Fitp support is disabled")
endif()

if (ENABLE_MOSQUITTO_CLIENT)
	file(GLOB MOSQUITTO_CLIENT_SOURCES
		${PROJECT_SOURCE_DIR}/net/GatewayMosquittoClient.cpp
		${PROJECT_SOURCE_DIR}/net/MosquittoClient.cpp
	)
	list(APPEND SOURCES ${MOSQUITTO_CLIENT_SOURCES})
else()
	message(STATUS "Mosquitto Client support is disabled")
endif()

if (ENABLE_IQRF)
	file(GLOB IQRF_SOURCES
		${PROJECT_SOURCE_DIR}/iqrf/DPAMessage.cpp
		${PROJECT_SOURCE_DIR}/iqrf/DPARequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/DPAResponse.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPACoordBondNodeRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPACoordBondedNodesRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/response/DPACoordBondedNodesResponse.cpp
	)
	list(APPEND SOURCES ${IQRF_SOURCES})
else()
	message(STATUS "IQRF support is disabled")
endif()

configure_file(core/Version.cpp.in ${PROJECT_BINARY_DIR}/core/Version.cpp)
list(APPEND SOURCES ${PROJECT_BINARY_DIR}/core/Version.cpp)

include_directories(
	${PROJECT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/base/src
)

add_library(BeeeOnGateway ${SOURCES})

add_executable(beeeon-gateway main.cpp)

set(LIBS
	${BLUETOOTH}
	${MOSQUITTO_CPP}
	${POCO_FOUNDATION}
	${POCO_SSL}
	${POCO_CRYPTO}
	${POCO_UTIL}
	${POCO_NET}
	${POCO_JSON}
	${POCO_XML}
	${PTHREAD}
	${UDEV}
	${OPENZWAVE_LIBRARY}
	${FITP_LIB}
	${GLIB_LDFLAGS}
	${GIO_UNIX_LDFLAGS}
)

target_link_libraries(beeeon-gateway
		-Wl,--whole-archive
		BeeeOnGateway
		BeeeOnBase
		-Wl,--no-whole-archive
		${LIBS}
)

install(TARGETS beeeon-gateway BeeeOnGateway
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
