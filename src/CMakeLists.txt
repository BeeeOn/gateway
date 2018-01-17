cmake_minimum_required (VERSION 2.8.11)
project (gateway-core CXX)

find_library (POCO_FOUNDATION PocoFoundation)
find_library (POCO_SSL PocoNetSSL)
find_library (POCO_CRYPTO PocoCrypto)
find_library (POCO_UTIL PocoUtil)
find_library (POCO_NET PocoNet)
find_library (POCO_JSON PocoJSON)
find_library (POCO_XML PocoXML)
find_library (PTHREAD pthread)

if(ENABLE_UDEV)
	find_library (UDEV udev)
endif()

if(ENABLE_MOSQUITTO_EXPORTER)
	find_library (MOSQUITTO_CPP mosquittopp)
endif()

if(ENABLE_BLUETOOTH_AVAILABILITY)
	find_library (BLUETOOTH bluetooth)
endif()

if(ENABLE_ZWAVE)
	find_package(OpenZWave REQUIRED)
	include_directories(${OPENZWAVE_INCLUDE_DIR})
endif()

if(ENABLE_FITP)
	find_library (FITP_LIB fitp)
endif()

file(GLOB SOURCES
	${PROJECT_SOURCE_DIR}/bluetooth/BluetoothListener.cpp
	${PROJECT_SOURCE_DIR}/commands/DeviceAcceptCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/DeviceSetValueCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/DeviceUnpairCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/GatewayListenCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/NewDeviceCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/ServerDeviceListCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/ServerDeviceListResult.cpp
	${PROJECT_SOURCE_DIR}/commands/ServerLastValueCommand.cpp
	${PROJECT_SOURCE_DIR}/commands/ServerLastValueResult.cpp
	${PROJECT_SOURCE_DIR}/core/AbstractDistributor.cpp
	${PROJECT_SOURCE_DIR}/core/AbstractCollector.cpp
	${PROJECT_SOURCE_DIR}/core/Answer.cpp
	${PROJECT_SOURCE_DIR}/core/AnswerQueue.cpp
	${PROJECT_SOURCE_DIR}/core/AsyncCommandDispatcher.cpp
	${PROJECT_SOURCE_DIR}/core/Command.cpp
	${PROJECT_SOURCE_DIR}/core/CommandDispatcher.cpp
	${PROJECT_SOURCE_DIR}/core/CommandDispatcherListener.cpp
	${PROJECT_SOURCE_DIR}/core/CommandHandler.cpp
	${PROJECT_SOURCE_DIR}/core/CommandRunner.cpp
	${PROJECT_SOURCE_DIR}/core/CommandSender.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceManager.cpp
	${PROJECT_SOURCE_DIR}/core/Distributor.cpp
	${PROJECT_SOURCE_DIR}/core/DistributorListener.cpp
	${PROJECT_SOURCE_DIR}/core/DongleDeviceManager.cpp
	${PROJECT_SOURCE_DIR}/core/Exporter.cpp
	${PROJECT_SOURCE_DIR}/core/GatewayInfo.cpp
	${PROJECT_SOURCE_DIR}/core/LoggingCollector.cpp
	${PROJECT_SOURCE_DIR}/core/PocoAnswerImpl.cpp
	${PROJECT_SOURCE_DIR}/core/Result.cpp
	${PROJECT_SOURCE_DIR}/credentials/Credentials.cpp
	${PROJECT_SOURCE_DIR}/credentials/CredentialsStorage.cpp
	${PROJECT_SOURCE_DIR}/credentials/FileCredentialsStorage.cpp
	${PROJECT_SOURCE_DIR}/credentials/PasswordCredentials.cpp
	${PROJECT_SOURCE_DIR}/credentials/PinCredentials.cpp
	${PROJECT_SOURCE_DIR}/exporters/NamedPipeExporter.cpp
	${PROJECT_SOURCE_DIR}/hotplug/AbstractHotplugMonitor.cpp
	${PROJECT_SOURCE_DIR}/hotplug/HotplugEvent.cpp
	${PROJECT_SOURCE_DIR}/hotplug/HotplugListener.cpp
	${PROJECT_SOURCE_DIR}/hotplug/PipeHotplugMonitor.cpp
	${PROJECT_SOURCE_DIR}/net/AbstractHTTPScanner.cpp
	${PROJECT_SOURCE_DIR}/net/SOAPMessage.cpp
	${PROJECT_SOURCE_DIR}/net/UPnP.cpp
	${PROJECT_SOURCE_DIR}/net/VPTHTTPScanner.cpp
	${PROJECT_SOURCE_DIR}/server/GWContextPoll.cpp
	${PROJECT_SOURCE_DIR}/server/GWMessageContext.cpp
	${PROJECT_SOURCE_DIR}/server/GWSOutputQueue.cpp
	${PROJECT_SOURCE_DIR}/server/GWServerConnector.cpp
	${PROJECT_SOURCE_DIR}/server/ServerAnswer.cpp
	${PROJECT_SOURCE_DIR}/util/CSVSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/DataReader.cpp
	${PROJECT_SOURCE_DIR}/util/DataWriter.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataParser.cpp
	${PROJECT_SOURCE_DIR}/util/NullSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/SensorDataFormatter.cpp
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

if(ENABLE_BLUETOOTH_AVAILABILITY)
	file(GLOB BLUETOOTH_AVAILABILITY_SOURCES
		${PROJECT_SOURCE_DIR}/bluetooth/BluetoothAvailabilityManager.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/BluetoothDevice.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/HciInfo.cpp
		${PROJECT_SOURCE_DIR}/bluetooth/HciInterface.cpp
	)
	list(APPEND SOURCES ${BLUETOOTH_AVAILABILITY_SOURCES})
	add_definitions(-DHAVE_HCI=1)
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
		${PROJECT_SOURCE_DIR}/philips/PhilipsHueBridge.cpp
		${PROJECT_SOURCE_DIR}/philips/PhilipsHueDeviceManager.cpp
		${PROJECT_SOURCE_DIR}/philips/PhilipsHueDimmableBulb.cpp
	)
	list(APPEND SOURCES ${PHILIPS_SOURCES})
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

if(ENABLE_MOSQUITTO_EXPORTER)
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
