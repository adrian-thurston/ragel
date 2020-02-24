# Package name
set(_PACKAGE_NAME colm)

# Read project configuration from ../configure.ac file
file(STRINGS ../configure.ac _PROJECT_CONFIGS
	REGEX "(COLM_VERSION=)|(COLM_PUBDATE=)")
foreach(_PROJECT_CONFIG ${_PROJECT_CONFIGS})
	if(_PROJECT_CONFIG MATCHES "COLM_VERSION=\"([^\"]+)")
		string(STRIP ${CMAKE_MATCH_1} COLM_VERSION)
	endif()
	if(_PROJECT_CONFIG MATCHES "COLM_PUBDATE=\"([^\"]+)")
		string(STRIP ${CMAKE_MATCH_1} COLM_PUBDATE)
	endif()
endforeach()

# Generate headers
configure_file(version.h.cmake.in version.h @ONLY)
configure_file(defs.h.cmake.in defs.h @ONLY)
configure_file(colm-config.cmake.in
"${PROJECT_BINARY_DIR}/${_PACKAGE_NAME}-config.cmake" @ONLY)

# Runtime headers
set(RUNTIME_HDR
	bytecode.h debug.h pool.h input.h
	pdarun.h map.h type.h tree.h struct.h program.h colm.h internal.h)

foreach(_hdr defs.h)
	list(APPEND RUNTIME_HDR "${CMAKE_CURRENT_BINARY_DIR}/${_hdr}")
endforeach()

# Other CMake modules
include(GNUInstallDirs)

# libcolm

add_library(libcolm
	map.c pdarun.c list.c input.c stream.c debug.c
	codevect.c pool.c string.c tree.c iter.c
	bytecode.c program.c struct.c commit.c
	print.c)

target_include_directories(libcolm
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../src>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(libcolm PROPERTIES
	OUTPUT_NAME colm)

# libprog

add_library(libprog
	buffer.h bytecode.h colm.h debug.h dotgen.h fsmcodegen.h fsmgraph.h
	input.h keyops.h map.h compiler.h
	parsetree.h pcheck.h pdacodegen.h pdagraph.h pdarun.h pool.h redbuild.h
	redfsm.h rtvector.h tree.h global.h colm.h parser.h cstring.h
	internal.h
	resolve.cc lookup.cc synthesis.cc parsetree.cc
	fsmstate.cc fsmbase.cc fsmattach.cc fsmmin.cc
	fsmgraph.cc pdagraph.cc pdabuild.cc pdacodegen.cc fsmcodegen.cc
	redfsm.cc fsmexec.cc redbuild.cc closure.cc fsmap.cc
	dotgen.cc pcheck.cc ctinput.cc declare.cc codegen.cc
	exports.cc compiler.cc parser.cc reduce.cc)

target_include_directories(libprog
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../aapl>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>)

set_target_properties(libprog PROPERTIES
	OUTPUT_NAME prog)

# bootstrap0

add_executable(bootstrap0
	consinit.cc consinit.h main.cc)

target_link_libraries(bootstrap0 libprog libcolm)

set_property(TARGET bootstrap0 APPEND PROPERTY
	COMPILE_DEFINITIONS CONS_INIT ${common_COMPILE_DEFINITIONS})

set_property(TARGET bootstrap0 APPEND PROPERTY
	COMPILE_FLAGS -fpermissive)

# bootstrap1

make_directory("${CMAKE_CURRENT_BINARY_DIR}/gen")

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gen/parse1.c"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if1.h"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if1.cc"
	COMMAND bootstrap0
	ARGS -c -o parse1.c -e if1.h -x if1.cc
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen")

add_executable(bootstrap1
	loadinit.h loadinit.cc main.cc
	"${CMAKE_CURRENT_BINARY_DIR}/gen/parse1.c"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if1.cc")

target_link_libraries(bootstrap1 libprog libcolm)

set_property(TARGET bootstrap1 APPEND PROPERTY
	COMPILE_DEFINITIONS LOAD_INIT ${common_COMPILE_DEFINITIONS})

# bootstrap2

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gen/parse2.c"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if2.h"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if2.cc"
	COMMAND bootstrap1
	ARGS -c -o parse2.c -e if2.h -x if2.cc "${CMAKE_CURRENT_LIST_DIR}/colm.lm"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen")

add_executable(bootstrap2
	main.cc loadboot2.cc loadfinal.h version.h
	"${CMAKE_CURRENT_BINARY_DIR}/gen/parse2.c"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if2.cc")

target_link_libraries(bootstrap2 libprog libcolm)

set_property(TARGET bootstrap2 APPEND PROPERTY
	COMPILE_DEFINITIONS LOAD_COLM ${common_COMPILE_DEFINITIONS})

# colm

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gen/parse3.c"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if3.h"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if3.cc"
	COMMAND bootstrap2
	ARGS -I "${CMAKE_CURRENT_LIST_DIR}" -c -o parse3.c -e if3.h -x if3.cc "${CMAKE_CURRENT_LIST_DIR}/prog.lm"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen")

add_executable(colm
	main.cc loadcolm.cc loadfinal.h version.h
	"${CMAKE_CURRENT_BINARY_DIR}/gen/parse3.c"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if3.cc")

if(BUILD_STANDALONE)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_link_libraries(colm -static)
	else()
		message(FATAL_ERROR "Unsupported toolset for standalone build.")
	endif()
endif()

target_link_libraries(colm libprog libcolm)

set_property(TARGET colm APPEND PROPERTY
	COMPILE_DEFINITIONS LOAD_COLM ${common_COMPILE_DEFINITIONS})

if(${PROJECT_NAME}_MAKE_INSTALL)
	if(NOT DEFINED CMAKE_INSTALL_CMAKEDIR)
		set(CMAKE_INSTALL_CMAKEDIR
			"${CMAKE_INSTALL_LIBDIR}/cmake/${_PACKAGE_NAME}"
			CACHE STRING "CMake packages")
	endif()
	install(FILES ${RUNTIME_HDR}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/colm")
	install(TARGETS libcolm colm
		EXPORT ${_PACKAGE_NAME}-targets
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
	install(EXPORT ${_PACKAGE_NAME}-targets
		NAMESPACE ${_PACKAGE_NAME}::
		DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
	export(EXPORT ${_PACKAGE_NAME}-targets
		NAMESPACE ${_PACKAGE_NAME}::
		FILE "${PROJECT_BINARY_DIR}/${_PACKAGE_NAME}-targets.cmake")
	include(CMakePackageConfigHelpers)
	write_basic_package_version_file(
		"${PROJECT_BINARY_DIR}/${_PACKAGE_NAME}-config-version.cmake"
		VERSION ${COLM_VERSION}
		COMPATIBILITY AnyNewerVersion)
	install(FILES
		"${PROJECT_BINARY_DIR}/${_PACKAGE_NAME}-config.cmake"
		"${PROJECT_BINARY_DIR}/${_PACKAGE_NAME}-config-version.cmake"
		DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
endif()
