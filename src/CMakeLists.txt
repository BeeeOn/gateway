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

set(LIBS
	${POCO_FOUNDATION}
	${POCO_SSL}
	${POCO_CRYPTO}
	${POCO_UTIL}
	${POCO_NET}
	${POCO_JSON}
	${POCO_XML}
	${PTHREAD}
)

find_package (PkgConfig)
if(PKG_CONFIG_FOUND)
	pkg_search_module (GLIB glib-2.0)
	pkg_search_module (GIO_UNIX gio-unix-2.0)
	pkg_search_module (BLUEZ bluez)
else()
	message(STATUS "PkgConfig is missing, glib, gio, bluez are not searched")
endif()

find_program (GDBUS_CODEGEN NAMES gdbus-codegen)

find_library (UDEV udev)
find_library (FITP_LIB fitp)
find_library (MOSQUITTO_CPP mosquittopp)
find_library (BLUETOOTH bluetooth)
find_package(OpenZWave)

if(GLIB_FOUND AND GIO_UNIX_FOUND AND GDBUS_CODEGEN AND BLUEZ_FOUND)
	include_directories(${GIO_UNIX_INCLUDE_DIRS})
	include_directories(${GLIB_INCLUDE_DIRS})
	set(HAS_DBUS_BLUEZ YES)
endif()

if(ENABLE_BLUETOOTH_AVAILABILITY OR ENABLE_BLE_SMART)
	set(WANTS_BLUETOOTH YES)
endif()

if(WANTS_BLUETOOTH AND HAS_DBUS_BLUEZ)
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

if(ENABLE_UCLIBCXX_FIXES)
	add_definitions(-DNO_std_to_string)
	set(CMAKE_CXX_FLAGS "-I${PROJECT_SOURCE_DIR}/../base/src/uclibc++ ${CMAKE_CXX_FLAGS}")
endif()

file(GLOB SOURCES
	${PROJECT_SOURCE_DIR}/bluetooth/HciListener.cpp
	${PROJECT_SOURCE_DIR}/commands/DeviceAcceptCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/DeviceSetValueCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/DeviceSearchCommand.cpp
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
	${PROJECT_SOURCE_DIR}/core/BasicDistributor.cpp
	${PROJECT_SOURCE_DIR}/core/Command.cpp
	${PROJECT_SOURCE_DIR}/core/CommandDispatcher.cpp
	${PROJECT_SOURCE_DIR}/core/CommandDispatcherListener.cpp
	${PROJECT_SOURCE_DIR}/core/CommandHandler.cpp
	${PROJECT_SOURCE_DIR}/core/CommandSender.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceCache.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceManager.cpp
	${PROJECT_SOURCE_DIR}/core/DevicePoller.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceStatusFetcher.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceStatusHandler.cpp
	${PROJECT_SOURCE_DIR}/core/Distributor.cpp
	${PROJECT_SOURCE_DIR}/core/DistributorListener.cpp
	${PROJECT_SOURCE_DIR}/core/DongleDeviceManager.cpp
	${PROJECT_SOURCE_DIR}/core/Exporter.cpp
	${PROJECT_SOURCE_DIR}/core/ExporterQueue.cpp
	${PROJECT_SOURCE_DIR}/core/FilesystemDeviceCache.cpp
	${PROJECT_SOURCE_DIR}/core/GatewayInfo.cpp
	${PROJECT_SOURCE_DIR}/core/LoggingCollector.cpp
	${PROJECT_SOURCE_DIR}/core/MemoryDeviceCache.cpp
	${PROJECT_SOURCE_DIR}/core/PollableDevice.cpp
	${PROJECT_SOURCE_DIR}/core/PollingKeeper.cpp
	${PROJECT_SOURCE_DIR}/core/PrefixCommand.cpp
	${PROJECT_SOURCE_DIR}/core/Result.cpp
	${PROJECT_SOURCE_DIR}/core/QueuingDistributor.cpp
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
	${PROJECT_SOURCE_DIR}/util/ChecksumSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/ChecksumSensorDataParser.cpp
	${PROJECT_SOURCE_DIR}/util/ColorBrightness.cpp
	${PROJECT_SOURCE_DIR}/util/CSVSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/DataWriter.cpp
	${PROJECT_SOURCE_DIR}/util/Journal.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataParser.cpp
	${PROJECT_SOURCE_DIR}/util/NullSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/SensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/SensorDataParser.cpp
	${PROJECT_SOURCE_DIR}/util/XmlTypeMappingParserHelper.cpp
	${PROJECT_SOURCE_DIR}/zwave/ZWaveListener.cpp
	${PROJECT_SOURCE_DIR}/zwave/ZWaveSerialProber.cpp
)

if(UDEV AND ENABLE_UDEV)
	file(GLOB UDEV_SOURCES ${PROJECT_SOURCE_DIR}/hotplug/UDevMonitor.cpp)
	list(APPEND SOURCES ${UDEV_SOURCES})
	list(APPEND LIBS ${UDEV})
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
	add_library(BeeeOnBelkinWemo ${BELKIN_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnBelkinWemo)
else()
	message(STATUS "Belkin Wemo support is disabled")
endif()

if(BLUETOOTH AND WANTS_BLUETOOTH)
	file(GLOB BLUETOOTH_SOURCES
		${PROJECT_SOURCE_DIR}/bluetooth/BluezHciInterface.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/DBusHciConnection.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/DBusHciInterface.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/HciConnection.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/HciInfo.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/HciInfoReporter.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/HciInterface.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/HciUtil.cpp
	)

	list(APPEND LIBS ${BLUETOOTH})

	if(HAS_DBUS_BLUEZ)
		list(APPEND BLUETOOTH_SOURCES
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

		if(GLIB_LDFLAGS)
			list(APPEND LIBS ${GLIB_LDFLAGS})
		endif()

		if(GIO_UNIX_LDFLAGS)
			list(APPEND LIBS ${GIO_UNIX_LDFLAGS})
		endif()
	endif()
	add_library(BeeeOnBluetooth ${BLUETOOTH_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnBluetooth)
	add_definitions(-DHAVE_HCI=1)
else()
	message(STATUS "Bluetooth support is disabled")
endif()

if(BLUETOOTH AND ENABLE_BLUETOOTH_AVAILABILITY)
	file(GLOB BLUETOOTH_AVAILABILITY_SOURCES
		${PROJECT_SOURCE_DIR}/bluetooth/BluetoothAvailabilityManager.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/BluetoothDevice.cpp
	)
	add_library(BeeeOnBluetoothAvailability ${BLUETOOTH_AVAILABILITY_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnBluetoothAvailability)
else()
	message(STATUS "Bluetooth Avaibility support is disabled")
endif()

if(BLUETOOTH AND ENABLE_BLE_SMART)
	file(GLOB BLE_SMART_SOURCES
		${PROJECT_SOURCE_DIR}/bluetooth/BeeWiDevice.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/BeeWiSmartClim.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/BeeWiSmartDoor.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/BeeWiSmartLite.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/BeeWiSmartMotion.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/BeeWiSmartWatt.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/BLESmartDevice.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/BLESmartDeviceManager.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/RevogiDevice.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/RevogiRGBLight.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/RevogiSmartCandle.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/RevogiSmartLite.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/RevogiSmartPlug.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/TabuLumenSmartLite.cpp
	)
	add_library(BeeeOnBLE ${BLE_SMART_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnBLE)
else()
	message(STATUS "Bluetooth Low Energy Smart support is disabled")
endif()

if(ENABLE_JABLOTRON)
	file(GLOB JABLOTRON_SOURCES
		${PROJECT_SOURCE_DIR}/jablotron/JablotronController.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceManager.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronGadget.cpp
		${PROJECT_SOURCE_DIR}/jablotron/JablotronReport.cpp
	)
	add_library(BeeeOnTurrisGadgets ${JABLOTRON_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnTurrisGadgets)
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
	add_library(BeeeOnPhilipsHue ${PHILIPS_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnPhilipsHue)
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
	add_library(BeeeOnVDev ${VIRTUAL_DEVICES_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnVDev)
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
	add_library(BeeeOnVPT ${VPT_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnVPT)
else()
	message(STATUS "VPT support is disabled")
endif()

if(OPENZWAVE_LIBRARY AND ENABLE_ZWAVE)
	file(GLOB ZWAVE_SOURCES
		${PROJECT_SOURCE_DIR}/zwave/AbstractZWaveNetwork.cpp
		${PROJECT_SOURCE_DIR}/zwave/AeotecZWaveMapperRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/ClimaxZWaveMapperRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/CompositeZWaveMapperRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/FibaroZWaveMapperRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/GenericZWaveMapperRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/SpecificZWaveMapperRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/ST02L1ZWaveMapperRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveDeviceManager.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveDriverEvent.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveMapperRegistry.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveNetwork.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveNode.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveNodeEvent.cpp
		${PROJECT_SOURCE_DIR}/zwave/ZWaveTypeMappingParser.cpp
	)
	add_library(BeeeOnZWave ${ZWAVE_SOURCES})
	add_definitions(-DHAVE_ZWAVE=1)
	list(APPEND MODULE_LIBS BeeeOnZWave)
endif()

if(OPENZWAVE_LIBRARY AND ENABLE_ZWAVE)
	file(GLOB OZW_SOURCES
		${PROJECT_SOURCE_DIR}/zwave/OZWCommand.cpp
		${PROJECT_SOURCE_DIR}/zwave/OZWNetwork.cpp
		${PROJECT_SOURCE_DIR}/zwave/OZWPocoLoggerAdapter.cpp
		${PROJECT_SOURCE_DIR}/zwave/OZWNotificationEvent.cpp
	)
	add_library(BeeeOnZWaveOZW ${OZW_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnZWaveOZW)
	add_definitions(-DHAVE_OPENZWAVE=1)
	include_directories(${OPENZWAVE_INCLUDE_DIR})
	list(APPEND LIBS ${OPENZWAVE_LIBRARY})
else()
	message(STATUS "OpenZWave support is disabled")
endif()

if(MOSQUITTO_CPP)
	file(GLOB MOSQUITTO_SUPPORT_SOURCES
		${PROJECT_SOURCE_DIR}/exporters/MosquittoExporter.cpp
		${PROJECT_SOURCE_DIR}/net/GatewayMosquittoClient.cpp
		${PROJECT_SOURCE_DIR}/net/MosquittoClient.cpp
	)
	add_library(BeeeOnMosquitto ${MOSQUITTO_SUPPORT_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnMosquitto)
	list(APPEND LIBS ${MOSQUITTO_CPP})
else()
	message(STATUS "Mosquitto support is disabled")
endif()

if (ENABLE_TESTING_CENTER)
	file(GLOB TESTING_CENTER_SOURCES
		${PROJECT_SOURCE_DIR}/core/TestingCenter.cpp
	)
	add_library(BeeeOnTestingCenter ${TESTING_CENTER_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnTestingCenter)
else()
	message(STATUS "Testing Center support is disabled")
endif()

if (ENABLE_PRESSURE_SENSOR)
	file(GLOB PRESSURE_SENSOR_SOURCES
		${PROJECT_SOURCE_DIR}/psdev/PressureSensorManager.cpp
	)
	add_library(BeeeOnPSDev ${PRESSURE_SENSOR_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnPSDev)
else()
	message(STATUS "Internal air pressure sensor support is disabled")
endif()

if(FITP_LIB AND ENABLE_FITP)
	file(GLOB FITP_SOURCES
		${PROJECT_SOURCE_DIR}/fitp/FitpDevice.cpp
		${PROJECT_SOURCE_DIR}/fitp/FitpDeviceManager.cpp
	)
	add_library(BeeeOnFitp ${FITP_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnFitp)
	list(APPEND LIBS ${FITP_LIB})
else()
	message(STATUS "Fitp support is disabled")
endif()

if (ENABLE_IQRF)
	file(GLOB IQRF_SOURCES
		${PROJECT_SOURCE_DIR}/iqrf/DPAMessage.cpp
		${PROJECT_SOURCE_DIR}/iqrf/DPARequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/DPAResponse.cpp
		${PROJECT_SOURCE_DIR}/iqrf/IQRFJsonMessage.cpp
		${PROJECT_SOURCE_DIR}/iqrf/IQRFJsonRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/IQRFJsonResponse.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPACoordBondNodeRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPACoordBondedNodesRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPACoordClearAllBondsRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPACoordDiscoveryRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPACoordRemoveNodeRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPANodeRemoveBondRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPAOSBatchRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPAOSPeripheralInfoRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/request/DPAOSRestartRequest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/response/DPACoordBondNodeResponse.cpp
		${PROJECT_SOURCE_DIR}/iqrf/response/DPACoordBondedNodesResponse.cpp
		${PROJECT_SOURCE_DIR}/iqrf/response/DPACoordRemoveNodeResponse.cpp
		${PROJECT_SOURCE_DIR}/iqrf/response/DPAOSPeripheralInfoResponse.cpp
	)
	add_library(BeeeOnIQRF ${IQRF_SOURCES})
	list(APPEND MODULE_LIBS BeeeOnIQRF)
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

target_link_libraries(beeeon-gateway
		-Wl,--whole-archive
		BeeeOnGateway
		BeeeOnBase
		${MODULE_LIBS}
		-Wl,--no-whole-archive
		${LIBS}
)

install(TARGETS beeeon-gateway BeeeOnGateway
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
