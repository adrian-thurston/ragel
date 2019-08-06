# Check type size
include(CheckTypeSize)
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)

# Check system headers
include(CheckIncludeFile)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)

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
configure_file(ragel-config.cmake.in
	"${PROJECT_BINARY_DIR}/${_PACKAGE_NAME}-config.cmake" @ONLY)

# Runtime headers
set(RUNTIME_HDR
	action.h fsmgraph.h ragel.h common.h
	gendata.h redfsm.h dot.h)

# Other CMake modules
include(GNUInstallDirs)

# libfsm

add_library(libfsm
	buffer.h codegen.h
	actloop.h actexp.h
	tables.h
	binary.h bingoto.h binbreak.h binvar.h
	flat.h flatgoto.h flatbreak.h flatvar.h
	switch.h switchgoto.h switchbreak.h switchvar.h
	goto.h gotoloop.h gotoexp.h
	ipgoto.h asm.h
	idbase.cc fsmstate.cc fsmbase.cc fsmattach.cc fsmmin.cc fsmgraph.cc
	fsmap.cc fsmcond.cc fsmnfa.cc common.cc redfsm.cc gendata.cc
	allocgen.cc codegen.cc
	actexp.cc binvar.cc
	tables.cc tabgoto.cc tabbreak.cc tabvar.cc
	binary.cc bingoto.cc binbreak.cc actloop.cc
	flat.cc flatgoto.cc flatbreak.cc flatvar.cc
	switch.cc switchgoto.cc switchbreak.cc switchvar.cc
	goto.cc gotoloop.cc gotoexp.cc ipgoto.cc
	dot.cc asm.cc)

target_include_directories(libfsm
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../aapl>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(libfsm PROPERTIES
	OUTPUT_NAME fsm)

# libragel

add_library(libragel
	# dist
	parsedata.h parsetree.h inputdata.h pcheck.h reducer.h rlscan.h load.h
	parsetree.cc longest.cc parsedata.cc inputdata.cc load.cc reducer.cc)

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
	rlreduce.lm)

add_custom_command(OUTPUT
	"${CMAKE_CURRENT_BINARY_DIR}/parse.c"
	"${CMAKE_CURRENT_BINARY_DIR}/rlreduce.cc"
	DEPENDS ${RAGEL_LM} #$(COLM_BINDEP)
	COMMAND colm::colm
	ARGS -c -b rlparseC
	-o "${CMAKE_CURRENT_BINARY_DIR}/parse.c"
	-m "${CMAKE_CURRENT_BINARY_DIR}/rlreduce.cc"
	rlparse.lm
	WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

add_executable(ragel
	main.cc
	"${CMAKE_CURRENT_BINARY_DIR}/parse.c"
	"${CMAKE_CURRENT_BINARY_DIR}/rlreduce.cc")

target_link_libraries(ragel libragel libfsm)

foreach(_SUBDIR host-ruby host-asm host-julia host-ocaml host-c host-d
		host-csharp host-go host-java host-rust host-crack host-js)
	add_subdirectory(${_SUBDIR})
endforeach()

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
	export(EXPORT ${_PACKAGE_NAME}-targets
		NAMESPACE ${_PACKAGE_NAME}::
		FILE "${PROJECT_BINARY_DIR}/${_PACKAGE_NAME}-targets.cmake")
	include(CMakePackageConfigHelpers)
	write_basic_package_version_file(
		"${PROJECT_BINARY_DIR}/${_PACKAGE_NAME}-config-version.cmake"
		VERSION ${${PROJECT_NAME}_VERSION}
		COMPATIBILITY AnyNewerVersion)
	install(FILES
		"${PROJECT_BINARY_DIR}/${_PACKAGE_NAME}-config.cmake"
		"${PROJECT_BINARY_DIR}/${_PACKAGE_NAME}-config-version.cmake"
		DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
endif()
