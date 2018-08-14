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

set(LIBS
	${POCO_FOUNDATION}
	${POCO_SSL}
	${POCO_CRYPTO}
	${POCO_UTIL}
	${POCO_NET}
	${POCO_JSON}
	${POCO_XML}
	${CPP_UNIT}
	${PTHREAD}
)

find_package (PkgConfig)
if (PKG_CONFIG_FOUND)
	pkg_search_module (GLIB glib-2.0)
	pkg_search_module (GIO_UNIX gio-unix-2.0)
endif()

find_library (UDEV udev)
find_library (FITP_LIB fitplib)
find_library (MOSQUITTO_CPP mosquittopp)
find_library (BLUETOOTH bluetooth)
find_package(OpenZWave)

if(GLIB_FOUND AND GIO_UNIX_FOUND)
	include_directories(${GIO_UNIX_INCLUDE_DIRS})
	include_directories(${GLIB_INCLUDE_DIRS})
	set(HAS_DBUS_BLUEZ YES)
endif()

file(GLOB TEST_SOURCES
	${PROJECT_SOURCE_DIR}/bluetooth/HciInterfaceTest.cpp
	${PROJECT_SOURCE_DIR}/core/AnswerQueueTest.cpp
	${PROJECT_SOURCE_DIR}/core/CommandDispatcherTest.cpp
	${PROJECT_SOURCE_DIR}/core/DeviceStatusFetcherTest.cpp
	${PROJECT_SOURCE_DIR}/core/DongleDeviceManagerTest.cpp
	${PROJECT_SOURCE_DIR}/core/ExporterQueueTest.cpp
	${PROJECT_SOURCE_DIR}/core/FilesystemDeviceCacheTest.cpp
	${PROJECT_SOURCE_DIR}/core/MemoryDeviceCacheTest.cpp
	${PROJECT_SOURCE_DIR}/core/QueuingDistributorTest.cpp
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

if(BLUETOOTH)
	list(APPEND LIBS ${BLUETOOTH})

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

if(OPENZWAVE_LIBRARY AND ENABLE_ZWAVE)
	include_directories(${OPENZWAVE_INCLUDE_DIR})

	file(GLOB ZWAVE_TEST_SOURCES
		${PROJECT_SOURCE_DIR}/zwave/ZWaveClassRegistryTest.cpp
	)
	list(APPEND TEST_SOURCES ${ZWAVE_TEST_SOURCES})
	list(APPEND LIBS ${OPENZWAVE_LIBRARY})
endif()

if(FITP_LIB AND ENABLE_FITP)
	file(GLOB FITP_TEST_SOURCES
		${PROJECT_SOURCE_DIR}/fitp/FitpDeviceTest.cpp
	)
	list(APPEND TEST_SOURCES ${FITP_TEST_SOURCES})
	list(APPEND LIBS ${FITP_LIB})
endif()

if(ENABLE_BLE_SMART)
	file(GLOB BLE_SMART_TEST_SOURCES
		${PROJECT_SOURCE_DIR}/bluetooth/BLESmartDeviceTest.cpp
	)
	list(APPEND TEST_SOURCES ${BLE_SMART_TEST_SOURCES})
endif()

if (ENABLE_IQRF)
	file(GLOB IQRF_TEST_SOURCES
		${PROJECT_SOURCE_DIR}/iqrf/DPAResponseTest.cpp
		${PROJECT_SOURCE_DIR}/iqrf/DPARequestTest.cpp
	)
	list(APPEND TEST_SOURCES ${IQRF_TEST_SOURCES})
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

if (MOSQUITTO_CPP)
list(APPEND LIBS ${MOSQUITTO_CPP})
endif()

if (UDEV)
list(APPEND LIBS ${UDEV})
endif()

if(GLIB_LDFLAGS)
list(APPEND LIBS ${GLIB_LDFLAGS})
endif()

if(GIO_UNIX_LDFLAGS)
list(APPEND LIBS ${GIO_UNIX_LDFLAGS})
endif()

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
