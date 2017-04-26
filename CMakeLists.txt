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

add_definitions(-DGIT_ID="${GIT_ID}" -std=c++11)

add_subdirectory(src)
add_subdirectory(base)
add_subdirectory(test)

install(FILES conf/gateway-startup.ini
	DESTINATION etc/beeeon/gateway)
install(DIRECTORY conf/config.d
	DESTINATION etc/beeeon/gateway)
install(FILES systemd/beeeon-gateway.service
	DESTINATION lib/systemd/system)
