cmake_minimum_required (VERSION 2.8.11)
project (gateway-tests CXX)

find_library (CPP_UNIT cppunit)
find_library (MOSQUITTO_CPP mosquittopp)
find_library (POCO_FOUNDATION PocoFoundation)
find_library (POCO_UTIL PocoUtil)
find_library (POCO_SSL PocoNetSSL)
find_library (POCO_CRYPTO PocoCrypto)
find_library (POCO_NET PocoNet)
find_library (POCO_JSON PocoJSON)
find_library (POCO_XML PocoXML)
find_library (PTHREAD pthread)
find_library (UDEV udev)

file(GLOB TEST_SOURCES
	${PROJECT_SOURCE_DIR}/core/AnswerQueueTest.cpp
	${PROJECT_SOURCE_DIR}/core/CommandDispatcherTest.cpp
	${PROJECT_SOURCE_DIR}/core/DongleDeviceManagerTest.cpp
	${PROJECT_SOURCE_DIR}/core/ExporterQueueTest.cpp
	${PROJECT_SOURCE_DIR}/core/QueuingDistributorTest.cpp
	${PROJECT_SOURCE_DIR}/credentials/CredentialsStorageTest.cpp
	${PROJECT_SOURCE_DIR}/credentials/CredentialsTest.cpp
	${PROJECT_SOURCE_DIR}/util/IncompleteTimestampTest.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataFormatterTest.cpp
)

include_directories(
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/../base/src
	${PROJECT_SOURCE_DIR}/../base/test
	${PROJECT_SOURCE_DIR}/../src
)

add_executable(test-suite-gateway
	${PROJECT_SOURCE_DIR}/../base/test/test.cpp
	${TEST_SOURCES}
)

set(LIBS
	${MOSQUITTO_CPP}
	${POCO_FOUNDATION}
	${POCO_SSL}
	${POCO_CRYPTO}
	${POCO_UTIL}
	${POCO_NET}
	${POCO_JSON}
	${POCO_XML}
	${CPP_UNIT}
	${PTHREAD}
	${UDEV}
)

target_link_libraries(test-suite-gateway
	-Wl,--whole-archive
	BeeeOnGateway
	BeeeOnBaseTest
	BeeeOnBase
	-Wl,--no-whole-archive
	${LIBS}
)

install(TARGETS test-suite-gateway
	RUNTIME DESTINATION share/beeeon/test-suite
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	CONFIGURATIONS Debug
)
