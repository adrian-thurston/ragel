cmake_minimum_required(VERSION 3.0)

# Read project configuration from configure.ac file
file(STRINGS configure.ac _PROJECT_CONFIGS
	REGEX "(AC_INIT\\()|(PUBDATE=)|(EXPECTED_COLM=)")
foreach(_PROJECT_CONFIG ${_PROJECT_CONFIGS})
	if(_PROJECT_CONFIG MATCHES "AC_INIT\\(([^,]+)")
		string(STRIP ${CMAKE_MATCH_1} PROJECT_NAME)
	endif()
	if(_PROJECT_CONFIG MATCHES "AC_INIT\\([^,]+,([^\\)]+)")
		string(STRIP ${CMAKE_MATCH_1} PROJECT_VERSION)
	endif()
	if(_PROJECT_CONFIG MATCHES "PUBDATE=\"([^\"]+)")
		string(STRIP ${CMAKE_MATCH_1} PROJECT_PUBDATE)
	endif()
	if(_PROJECT_CONFIG MATCHES "EXPECTED_COLM=(.+)")
		string(STRIP ${CMAKE_MATCH_1} PROJECT_EXPECTED_COLM)
	endif()
endforeach()

project(${PROJECT_NAME}
	VERSION ${PROJECT_VERSION}
	LANGUAGES C CXX)

find_package(colm ${PROJECT_EXPECTED_COLM} REQUIRED)

set(${PROJECT_NAME}_MAKE_INSTALL ON CACHE BOOL
    "Set to OFF to disable install rules (default is ON)")

add_subdirectory(src)
