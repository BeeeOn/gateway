find_path(OPENZWAVE_INCLUDE_DIR
	NAMES Manager.h
	PATHS
		/usr/include/
		/usr/local/include/
	PATH_SUFFIXES openzwave
)
find_library(OPENZWAVE_LIBRARY
	NAMES openzwave
	PATHS
		/usr/lib
		/usr/lib64
		/usr/local/lib
		/usr/local/lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenZWave
	FOUND_VAR OPENZWAVE_FOUND
	REQUIRED_VARS
		OPENZWAVE_LIBRARY
		OPENZWAVE_INCLUDE_DIR
)

if(OPENZWAVE_FOUND)
	set(OPENZWAVE_INCLUDE_DIR ${OPENZWAVE_INCLUDE_DIR})
	set(OPENZWAVE_LIBRARY ${OPENZWAVE_LIBRARY})
endif()
