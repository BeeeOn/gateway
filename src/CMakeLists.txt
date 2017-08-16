cmake_minimum_required (VERSION 2.8.11)
project (gateway-core CXX)

find_library (MOSQUITTO_CPP mosquittopp)
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

find_package(OpenZWave REQUIRED)
include_directories(${OPENZWAVE_INCLUDE_DIR})

file(GLOB SOURCES
	${PROJECT_SOURCE_DIR}/belkin/BelkinWemoDeviceManager.cpp
	${PROJECT_SOURCE_DIR}/belkin/BelkinWemoSwitch.cpp
	${PROJECT_SOURCE_DIR}/bluetooth/BluetoothAvailabilityManager.cpp
	${PROJECT_SOURCE_DIR}/bluetooth/BluetoothDevice.cpp
	${PROJECT_SOURCE_DIR}/bluetooth/HciInterface.cpp
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
	${PROJECT_SOURCE_DIR}/core/Answer.cpp
	${PROJECT_SOURCE_DIR}/core/AnswerQueue.cpp
	${PROJECT_SOURCE_DIR}/core/BasicDistributor.cpp
	${PROJECT_SOURCE_DIR}/core/Command.cpp
	${PROJECT_SOURCE_DIR}/core/CommandDispatcher.cpp
	${PROJECT_SOURCE_DIR}/core/CommandHandler.cpp
	${PROJECT_SOURCE_DIR}/core/CommandProgressHandler.cpp
	${PROJECT_SOURCE_DIR}/core/CommandRunner.cpp
	${PROJECT_SOURCE_DIR}/core/CommandSender.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceManager.cpp
	${PROJECT_SOURCE_DIR}/core/DistributorListener.cpp
	${PROJECT_SOURCE_DIR}/core/DongleDeviceManager.cpp
	${PROJECT_SOURCE_DIR}/core/Exporter.cpp
	${PROJECT_SOURCE_DIR}/core/ExporterQueue.cpp
	${PROJECT_SOURCE_DIR}/core/GatewayInfo.cpp
	${PROJECT_SOURCE_DIR}/core/PocoAnswerImpl.cpp
	${PROJECT_SOURCE_DIR}/core/PocoCommandDispatcher.cpp
	${PROJECT_SOURCE_DIR}/core/QueuingDistributor.cpp
	${PROJECT_SOURCE_DIR}/core/Result.cpp
	${PROJECT_SOURCE_DIR}/credentials/Credentials.cpp
	${PROJECT_SOURCE_DIR}/credentials/CredentialsStorage.cpp
	${PROJECT_SOURCE_DIR}/credentials/FileCredentialsStorage.cpp
	${PROJECT_SOURCE_DIR}/credentials/PasswordCredentials.cpp
	${PROJECT_SOURCE_DIR}/credentials/PinCredentials.cpp
	${PROJECT_SOURCE_DIR}/exporters/MosquittoExporter.cpp
	${PROJECT_SOURCE_DIR}/exporters/NamedPipeExporter.cpp
	${PROJECT_SOURCE_DIR}/hotplug/AbstractHotplugMonitor.cpp
	${PROJECT_SOURCE_DIR}/hotplug/HotplugEvent.cpp
	${PROJECT_SOURCE_DIR}/hotplug/HotplugListener.cpp
	${PROJECT_SOURCE_DIR}/hotplug/PipeHotplugMonitor.cpp
	${PROJECT_SOURCE_DIR}/jablotron/JablotronDevice.cpp
	${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceAC88.cpp
	${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceJA82SH.cpp
	${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceJA83P.cpp
	${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceJA85ST.cpp
	${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceManager.cpp
	${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceOpenClose.cpp
	${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceRC86K.cpp
	${PROJECT_SOURCE_DIR}/jablotron/JablotronDeviceTP82N.cpp
	${PROJECT_SOURCE_DIR}/net/AbstractHTTPScanner.cpp
	${PROJECT_SOURCE_DIR}/net/SOAPMessage.cpp
	${PROJECT_SOURCE_DIR}/net/UPnP.cpp
	${PROJECT_SOURCE_DIR}/net/VPTHTTPScanner.cpp
	${PROJECT_SOURCE_DIR}/util/CSVSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/NullSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/SensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/vdev/VirtualModule.cpp
	${PROJECT_SOURCE_DIR}/vdev/VirtualDevice.cpp
	${PROJECT_SOURCE_DIR}/vdev/VirtualDeviceManager.cpp
	${PROJECT_SOURCE_DIR}/zwave/DefaultZWaveDeviceInfo.cpp
	${PROJECT_SOURCE_DIR}/zwave/DefaultZWaveDeviceInfoRegistry.cpp
	${PROJECT_SOURCE_DIR}/zwave/GenericZWaveDeviceInfoRegistry.cpp
	${PROJECT_SOURCE_DIR}/zwave/VendorZWaveDeviceInfoRegistry.cpp
	${PROJECT_SOURCE_DIR}/zwave/ZWaveClassRegistry.cpp
	${PROJECT_SOURCE_DIR}/zwave/ZWaveDeviceInfo.cpp
	${PROJECT_SOURCE_DIR}/zwave/ZWaveDeviceManager.cpp
	${PROJECT_SOURCE_DIR}/zwave/ZWaveDriver.cpp
	${PROJECT_SOURCE_DIR}/zwave/ZWaveNodeInfo.cpp
	${PROJECT_SOURCE_DIR}/zwave/ZWavePocoLoggerAdapter.cpp
)

if(ENABLE_UDEV)
	file(GLOB UDEV_SOURCES ${PROJECT_SOURCE_DIR}/hotplug/UDevMonitor.cpp)
	list(APPEND SOURCES ${UDEV_SOURCES})
else()
	message(STATUS "udev support is disabled")
endif()

file(GLOB TESTING_SOURCES
	${PROJECT_SOURCE_DIR}/core/TestingCenter.cpp
)

include_directories(
	${PROJECT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/base/src
)

add_library(BeeeOnGateway ${SOURCES})
add_library(BeeeOnTesting ${TESTING_SOURCES})

add_executable(beeeon-gateway main.cpp)

set(LIBS
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
)

target_link_libraries(beeeon-gateway
		-Wl,--whole-archive
		BeeeOnGateway
		BeeeOnTesting
		BeeeOnBase
		-Wl,--no-whole-archive
		${LIBS}
)

install(TARGETS beeeon-gateway BeeeOnGateway BeeeOnTesting
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
