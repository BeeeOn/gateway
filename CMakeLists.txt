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

add_definitions(-DGIT_ID="${GIT_ID}")

add_subdirectory(src)
add_subdirectory(base)
add_subdirectory(test)

install(FILES conf/gateway-startup.ini
	DESTINATION etc/beeeon-gateway)
install(DIRECTORY conf/config.d
	DESTINATION etc/beeeon-gateway)
