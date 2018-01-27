# Check type size
include(CheckTypeSize)
check_type_size("long" SIZEOF_LONG)
check_type_size("unsigned long" SIZEOF_ULONG)

if(SIZEOF_ULONG LESS CMAKE_SIZEOF_VOID_P)
	message(FATAL_ERROR "Check CodeVect::insertTree implementation, because sizeof(word_t) < sizeof(void*)!")
endif()

# Check system headers
include(CheckIncludeFile)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(unistd.h HAVE_UNISTD_H)

# Prepare settings
string(TOLOWER ${PROJECT_NAME} _PACKAGE_NAME)
if("${CMAKE_BUILD_TYPE}" MATCHES "[Dd][Ee][Bb]")
	set(DEBUG 1)
endif()
set(VERSION "${PROJECT_VERSION}")
set(PUBDATE "${PROJECT_PUBDATE}")

set(common_COMPILE_DEFINITIONS PREFIX="${CMAKE_INSTALL_PREFIX}")

# Generate headers
configure_file(version.h.cmake.in version.h @ONLY)
configure_file(config.h.cmake.in config.h @ONLY)
configure_file(defs.h.cmake.in defs.h @ONLY)
configure_file(colm-config.cmake.in ${_PACKAGE_NAME}-config.cmake @ONLY)

# Runtime headers
set(RUNTIME_HDR
	bytecode.h debug.h pool.h input.h
	pdarun.h map.h type.h tree.h struct.h program.h colm.h internal.h)

# Buildtime headers stub
foreach(_hdr ${RUNTIME_HDR} rtvector.h)
	file(GENERATE
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/colm/${_hdr}"
		CONTENT "#include \"${CMAKE_CURRENT_LIST_DIR}/${_hdr}\"\n")
endforeach()
foreach(_hdr config.h defs.h)
	file(GENERATE
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/colm/${_hdr}"
		CONTENT "#include \"../../${_hdr}\"\n")
	list(APPEND RUNTIME_HDR "${CMAKE_CURRENT_BINARY_DIR}/${_hdr}")
endforeach()

# Other CMake modules
include(GNUInstallDirs)

# libcolm

add_library(libcolm
	map.c pdarun.c list.c input.c debug.c
	codevect.c pool.c string.c tree.c iter.c
	bytecode.c program.c struct.c commit.c
	print.c)

target_include_directories(libcolm
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
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
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../aapl>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

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

# colm

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gen/parse2.c"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if2.h"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if2.cc"
	COMMAND bootstrap1
	ARGS -c -o parse2.c -e if2.h -x if2.cc "${CMAKE_CURRENT_LIST_DIR}/colm.lm"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen")

add_executable(colm
	loadcolm.h loadcolm.cc main.cc
	"${CMAKE_CURRENT_BINARY_DIR}/gen/parse2.c"
	"${CMAKE_CURRENT_BINARY_DIR}/gen/if2.cc")

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
