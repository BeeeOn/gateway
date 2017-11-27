cmake_minimum_required (VERSION 2.8.11)
project (gateway CXX)

if(DEFINED ENV{GIT_ID})
	set (GIT_ID $ENV{GIT_ID})
else()
	exec_program(
		"git"
		${CMAKE_CURRENT_SOURCE_DIR}
		ARGS "describe --always --tags"
		OUTPUT_VARIABLE GIT_ID)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	message("Debug build")
	add_definitions(-D_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
	message("Release build")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_definitions(-std=c++11 -Wall -pedantic -Wextra)

option(ENABLE_UDEV "Enable support of udev" ON)
option(ENABLE_BELKIN_WEMO "Enable support of Belkin Wemo" ON)
option(ENABLE_BLUETOOTH_AVAILABILITY "Enable support of Bluetooth Availability" ON)
option(ENABLE_JABLOTRON "Enable support of Jablotron" ON)
option(ENABLE_PHILIPS_HUE "Enable support of Philips Hue" ON)
option(ENABLE_VIRTUAL_DEVICES "Enable support of Virtual Devices" ON)
option(ENABLE_VPT "Enable support of VPT" ON)
option(ENABLE_ZWAVE "Enable support of ZWave" ON)
option(ENABLE_MOSQUITTO_EXPORTER "Enable support of Mosquitto Exporting" ON)
option(ENABLE_BASIC_DISTRIBUTOR "Enable support of Basic Distributor" ON)
option(ENABLE_QUEUING_DISTRIBUTOR "Enable support of Queuing Distributor" ON)
option(ENABLE_TESTING_CENTER "Enable support of Testing Center" ON)
option(ENABLE_PRESSURE_SENSOR "Enable support of internal air pressure sensor" ON)

add_subdirectory(src)
add_subdirectory(base)
add_subdirectory(test)

find_package(Doxygen)

if(DOXYGEN_FOUND)
	set(DOXYGEN_IN  ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
	set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
	configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT})
	add_custom_target(doc
		COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen"
		VERBATIM)
endif()

install(FILES conf/gateway-startup.ini
	DESTINATION etc/beeeon/gateway)
install(FILES conf/virtual-devices.ini
	DESTINATION etc/beeeon/gateway)
install(DIRECTORY conf/config.d
	DESTINATION etc/beeeon/gateway)
install(FILES systemd/beeeon-gateway.service
	DESTINATION lib/systemd/system)
install(DIRECTORY udev/rules.d
	DESTINATION etc/udev)
