cmake_minimum_required(VERSION 3.0)

# Read project configuration from configure.ac file
file(STRINGS configure.ac _PROJECT_CONFIGS
	REGEX "(AC_INIT\\()|(PUBDATE=)")
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
endforeach()

project(${PROJECT_NAME}
	VERSION ${PROJECT_VERSION}
	LANGUAGES C CXX)

set(${PROJECT_NAME}_MAKE_INSTALL ON CACHE BOOL
	"Set to OFF to disable install rules (default is ON)")

set(${PROJECT_NAME}_BUILD_EXAMPLES OFF CACHE BOOL
	"Set to ON to build examples (default is OFF)")

# Check type size
include(CheckTypeSize)
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)
check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)
check_type_size("unsigned long long" SIZEOF_UNSIGNED_LONG_LONG)
check_type_size("void *" SIZEOF_VOID_P)

# Check system headers
include(CheckIncludeFile)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(unistd.h HAVE_UNISTD_H)

# Prepare settings
if("${CMAKE_BUILD_TYPE}" MATCHES "[Dd][Ee][Bb]")
	set(DEBUG 1)
endif()
set(VERSION "${PROJECT_VERSION}")
set(PUBDATE "${PROJECT_PUBDATE}")

# Common compile definitions
set(common_COMPILE_DEFINITIONS PREFIX="${CMAKE_INSTALL_PREFIX}")

add_subdirectory(src)

add_subdirectory(colm)
add_library(colm::libcolm ALIAS libcolm)
add_executable(colm::colm ALIAS colm)

add_subdirectory(ragel)

if(${PROJECT_NAME}_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()
