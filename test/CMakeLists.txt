cmake_minimum_required (VERSION 2.8.11)
project (gateway-tests CXX)

find_library (CPP_UNIT cppunit)
find_library (POCO_FOUNDATION PocoFoundation)
find_library (POCO_UTIL PocoUtil)
find_library (POCO_SSL PocoNetSSL)
find_library (POCO_CRYPTO PocoCrypto)
find_library (POCO_NET PocoNet)
find_library (POCO_JSON PocoJSON)
find_library (POCO_XML PocoXML)
find_library (PTHREAD pthread)
find_package (PkgConfig REQUIRED)
pkg_search_module (GLIB REQUIRED glib-2.0)
pkg_search_module (GIO_UNIX REQUIRED gio-unix-2.0)

if(GLIB_FOUND AND GIO_UNIX_FOUND)
	include_directories(${GIO_UNIX_INCLUDE_DIRS})
	include_directories(${GLIB_INCLUDE_DIRS})
	set(HAS_DBUS_BLUEZ YES)
endif()

if(ENABLE_UDEV)
	find_library (UDEV udev)
endif()

if(ENABLE_MOSQUITTO_EXPORTER)
	find_library (MOSQUITTO_CPP mosquittopp)
endif()

if(ENABLE_BLUETOOTH)
	find_library (BLUETOOTH bluetooth)
endif()

if(ENABLE_ZWAVE)
	find_package(OpenZWave REQUIRED)
	include_directories(${OPENZWAVE_INCLUDE_DIR})
endif()

if(ENABLE_FITP)
	find_library (FITP_LIB fitplib)
endif()

file(GLOB TEST_SOURCES
	${PROJECT_SOURCE_DIR}/bluetooth/HciInterfaceTest.cpp
	${PROJECT_SOURCE_DIR}/core/AnswerQueueTest.cpp
	${PROJECT_SOURCE_DIR}/core/CommandDispatcherTest.cpp
	${PROJECT_SOURCE_DIR}/core/DongleDeviceManagerTest.cpp
	${PROJECT_SOURCE_DIR}/core/FilesystemDeviceCacheTest.cpp
	${PROJECT_SOURCE_DIR}/core/MemoryDeviceCacheTest.cpp
	${PROJECT_SOURCE_DIR}/core/QueuingExporterTest.cpp
	${PROJECT_SOURCE_DIR}/credentials/CredentialsStorageTest.cpp
	${PROJECT_SOURCE_DIR}/credentials/CredentialsTest.cpp
	${PROJECT_SOURCE_DIR}/exporters/JournalQueuingStrategyTest.cpp
	${PROJECT_SOURCE_DIR}/exporters/RecoverableJournalQueuingStrategyTest.cpp
	${PROJECT_SOURCE_DIR}/util/ColorBrightnessTest.cpp
	${PROJECT_SOURCE_DIR}/util/CSVSensorDataFormatterTest.cpp
	${PROJECT_SOURCE_DIR}/util/DataWriterTest.cpp
	${PROJECT_SOURCE_DIR}/util/DataReaderTest.cpp
	${PROJECT_SOURCE_DIR}/util/JournalTest.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataFormatterTest.cpp
	${PROJECT_SOURCE_DIR}/util/JSONSensorDataParserTest.cpp
)

if(ENABLE_BLUETOOTH)
	if(HAS_DBUS_BLUEZ)
		file(GLOB BLUETOOTH_SOURCES
			${PROJECT_SOURCE_DIR}/bluetooth/GlibPtrTest.cpp
		)
		list(APPEND TEST_SOURCES ${BLUETOOTH_SOURCES})
	endif()
endif()

if(ENABLE_VPT)
	file(GLOB VPT_TEST_SOURCES
		${PROJECT_SOURCE_DIR}/vpt/VPTDeviceTest.cpp
	)
	list(APPEND TEST_SOURCES ${VPT_TEST_SOURCES})
endif()

if(ENABLE_ZWAVE)
	file(GLOB ZWAVE_TEST_SOURCES
		${PROJECT_SOURCE_DIR}/zwave/ZWaveClassRegistryTest.cpp
	)
	list(APPEND TEST_SOURCES ${ZWAVE_TEST_SOURCES})
endif()

if (ENABLE_QUEUING_DISTRIBUTOR)
	file(GLOB QUEUING_DISTRIBUTOR_TEST_SOURCES
		${PROJECT_SOURCE_DIR}/core/ExporterQueueTest.cpp
		${PROJECT_SOURCE_DIR}/core/QueuingDistributorTest.cpp
	)
	list(APPEND TEST_SOURCES ${QUEUING_DISTRIBUTOR_TEST_SOURCES})
endif()

if(ENABLE_FITP)
	file(GLOB FITP_TEST_SOURCES
		${PROJECT_SOURCE_DIR}/fitp/FitpDeviceTest.cpp
	)
	list(APPEND TEST_SOURCES ${FITP_TEST_SOURCES})
endif()

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
	${BLUETOOTH}
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
	${OPENZWAVE_LIBRARY}
	${FITP_LIB}
	${GLIB_LDFLAGS}
	${GIO_UNIX_LDFLAGS}
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
