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

file(GLOB SOURCES
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
	${PROJECT_SOURCE_DIR}/core/Exporter.cpp
	${PROJECT_SOURCE_DIR}/core/GatewayInfo.cpp
	${PROJECT_SOURCE_DIR}/core/ModuleType.cpp
	${PROJECT_SOURCE_DIR}/core/PocoAnswerImpl.cpp
	${PROJECT_SOURCE_DIR}/core/PocoCommandDispatcher.cpp
	${PROJECT_SOURCE_DIR}/core/Result.cpp
	${PROJECT_SOURCE_DIR}/core/VirtualSensor.cpp
	${PROJECT_SOURCE_DIR}/credentials/Credentials.cpp
	${PROJECT_SOURCE_DIR}/credentials/CredentialsStorage.cpp
	${PROJECT_SOURCE_DIR}/credentials/FileCredentialsStorage.cpp
	${PROJECT_SOURCE_DIR}/credentials/PasswordCredentials.cpp
	${PROJECT_SOURCE_DIR}/credentials/PinCredentials.cpp
	${PROJECT_SOURCE_DIR}/exporters/NamedPipeExporter.cpp
	${PROJECT_SOURCE_DIR}/model/ModuleID.cpp
	${PROJECT_SOURCE_DIR}/model/SensorValue.cpp
	${PROJECT_SOURCE_DIR}/net/AbstractHTTPScanner.cpp
	${PROJECT_SOURCE_DIR}/net/SOAPMessage.cpp
	${PROJECT_SOURCE_DIR}/net/UPnP.cpp
	${PROJECT_SOURCE_DIR}/net/VPTHTTPScanner.cpp
	${PROJECT_SOURCE_DIR}/util/CSVSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/NullSensorDataFormatter.cpp
	${PROJECT_SOURCE_DIR}/util/SensorDataFormatter.cpp
)

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
	${POCO_FOUNDATION}
	${POCO_SSL}
	${POCO_CRYPTO}
	${POCO_UTIL}
	${POCO_NET}
	${POCO_JSON}
	${POCO_XML}
	${PTHREAD}
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
