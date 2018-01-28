# Check type size
include(CheckTypeSize)
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)

# Prepare settings
string(TOLOWER ${PROJECT_NAME} _PACKAGE_NAME)
if("${CMAKE_BUILD_TYPE}" MATCHES "[Dd][Ee][Bb]")
	set(DEBUG 1)
endif()
set(VERSION "${PROJECT_VERSION}")
set(PUBDATE "${PROJECT_PUBDATE}")

set(common_COMPILE_DEFINITIONS PREFIX="${CMAKE_INSTALL_PREFIX}")

## Generate headers
configure_file(version.h.cmake.in version.h @ONLY)
configure_file(config.h.cmake.in config.h @ONLY)
configure_file(ragel-config.cmake.in ${_PACKAGE_NAME}-config.cmake @ONLY)

# Runtime headers
set(RUNTIME_HDR
	action.h fsmgraph.h ragel.h common.h
	gendata.h redfsm.h dot.h)

# Other CMake modules
include(GNUInstallDirs)

# libfsm

add_library(libfsm
	buffer.h codegen.h binary.h bingoto.h bingotoloop.h bingotoexp.h binvar.h
	binvarloop.h binvarexp.h flat.h flatgotoloop.h flatgotoexp.h flatvar.h
	flatvarloop.h flatvarexp.h goto.h gotoloop.h gotoexp.h
	ipgoto.h asm.h
	idbase.cc fsmstate.cc fsmbase.cc fsmattach.cc fsmmin.cc fsmgraph.cc
	fsmap.cc fsmcond.cc fsmnfa.cc common.cc redfsm.cc gendata.cc
	allocgen.cc codegen.cc binary.cc bingotoloop.cc bingotoexp.cc binvar.cc
	binvarloop.cc binvarexp.cc flat.cc flatgotoloop.cc flatgotoexp.cc
	flatvar.cc flatvarloop.cc flatvarexp.cc goto.cc gotoloop.cc
	gotoexp.cc ipgoto.cc dot.cc asm.cc)

target_include_directories(libfsm
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../aapl>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(libfsm PROPERTIES
	OUTPUT_NAME fsm)

# libragel

add_custom_command(OUTPUT
	"${CMAKE_CURRENT_BINARY_DIR}/ldparse.c"
	"${CMAKE_CURRENT_BINARY_DIR}/ldreduce.cc"
	DEPENDS langdesc.lm #$(COLM_BINDEP)
	COMMAND colm::colm
	ARGS -c -b ld_parse
	-o "${CMAKE_CURRENT_BINARY_DIR}/ldparse.c"
	-m "${CMAKE_CURRENT_BINARY_DIR}/ldreduce.cc"
	langdesc.lm
	WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

add_library(libragel
	# dist
	parsedata.h parsetree.h inputdata.h pcheck.h reducer.h rlscan.h load.h
	parsetree.cc parsedata.cc inputdata.cc load.cc reducer.cc
	# nodist
	"${CMAKE_CURRENT_BINARY_DIR}/ldparse.c"
	"${CMAKE_CURRENT_BINARY_DIR}/ldreduce.cc")

target_link_libraries(libragel colm::libcolm)

target_include_directories(libragel
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../aapl>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(libragel PROPERTIES
	OUTPUT_NAME ragel)

set_property(TARGET libragel APPEND PROPERTY
	COMPILE_DEFINITIONS BINDIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")

# ragel program

set(RAGEL_LM
	rlparse.lm
	ragel.lm
	host.lm
	reducer.lm)

add_custom_command(OUTPUT
	"${CMAKE_CURRENT_BINARY_DIR}/parse.c"
	"${CMAKE_CURRENT_BINARY_DIR}/rlreduce.cc"
	DEPENDS ${RAGEL_LM} #$(COLM_BINDEP)
	COMMAND colm::colm
	ARGS -c -b rl_parse
	-o "${CMAKE_CURRENT_BINARY_DIR}/parse.c"
	-m "${CMAKE_CURRENT_BINARY_DIR}/rlreduce.cc"
	rlparse.lm
	WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

add_executable(ragel
	main.cc
	"${CMAKE_CURRENT_BINARY_DIR}/parse.c"
	"${CMAKE_CURRENT_BINARY_DIR}/rlreduce.cc")

target_link_libraries(ragel libragel libfsm)

if(${PROJECT_NAME}_MAKE_INSTALL)
	if(NOT DEFINED CMAKE_INSTALL_CMAKEDIR)
		set(CMAKE_INSTALL_CMAKEDIR
			"${CMAKE_INSTALL_LIBDIR}/cmake/${_PACKAGE_NAME}"
			CACHE STRING "CMake packages")
	endif()
	install(FILES ${RUNTIME_HDR}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ragel")
	install(TARGETS libfsm libragel ragel
		EXPORT ${_PACKAGE_NAME}-targets
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
	install(EXPORT ${_PACKAGE_NAME}-targets
		NAMESPACE ${_PACKAGE_NAME}::
		DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
	include(CMakePackageConfigHelpers)
	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/${_PACKAGE_NAME}-config-version.cmake"
		VERSION ${${PROJECT_NAME}_VERSION}
		COMPATIBILITY AnyNewerVersion)
	install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/${_PACKAGE_NAME}-config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/${_PACKAGE_NAME}-config-version.cmake"
		DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
endif()
