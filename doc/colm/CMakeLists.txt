#
# The colm manual, rendered to HTML with Asciidoctor. This is the cmake
# equivalent of Makefile.am beside it, which builds the manual under
# --enable-manual. Only included when COLM_BUILD_MANUAL is on.
#

find_program(ASCIIDOCTOR asciidoctor)
if(NOT ASCIIDOCTOR)
	message(FATAL_ERROR
		"COLM_BUILD_MANUAL is on but asciidoctor was not found. Install "
		"Asciidoctor, or turn the manual off with -DCOLM_BUILD_MANUAL=OFF.")
endif()

# The install page and the FAQ show versions, which the top-level
# CMakeLists.txt reads out of configure.ac.
set(MANUAL_OPTIONS
	-b html5
	-a max-width=800px
	-a source-highlighter=rouge
	-a webfonts!
	-a suite-release=${SUITE_VERSION}
	-a colm-version=${COLM_VERSION}
	-a "colm-pubdate=${COLM_PUBDATE}")

set(MANUAL_PAGES
	0_00_welcome 0_06_scope 1_05_variables
	3_00_language_transformation 8_00_advanced_topics
	9_00_q_and_a 0_01_installing 1_00_language_overview
	1_06_iterators 3_01_lex 8_01_embedding
	0_02_vim_syntax 1_01_statements 2_00_types
	3_02_parse 8_02_modularity 0_03_commandline
	1_02_expressions 2_01_basic 3_03_match
	8_03_performance 0_04_hello_world 1_03_control_flow
	2_02_list_map_struct_alias 3_04_undo 8_04_error_handling
	0_05_fizzbuzz 1_04_functions 2_03_def
	3_05_reduction 8_05_contributing)

# The example programs, with their output (.exp) and input (.in), which the
# pages include. Every page is rebuilt when one of them changes, which is
# simpler than tracking which page includes which.
set(MANUAL_CODE
	code/assign.lm           code/assign.exp           code/assign.in
	code/figure_44.lm        code/figure_44.exp
	code/fizzbuzz.lm         code/fizzbuzz.exp
	code/hello_world.lm      code/hello_world.exp
	code/hello_world_ext.lm  code/hello_world_ext.exp
	code/lex_basics.lm       code/lex_basics.exp       code/lex_basics.in
	code/lex_choose.lm       code/lex_choose.exp       code/lex_choose.in
	code/lex_heredoc.lm      code/lex_heredoc.exp      code/lex_heredoc.in
	code/lex_regions.lm      code/lex_regions.exp      code/lex_regions.in
	code/lex_rl.lm           code/lex_rl.exp           code/lex_rl.in
	code/nested_scope.lm     code/nested_scope.exp
	code/poker.lm            code/poker.exp
	code/reference.lm        code/reference.exp
	code/scope.lm            code/scope.exp)
list(TRANSFORM MANUAL_CODE PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

set(MANUAL_HTML)
foreach(page ${MANUAL_PAGES})
	set(adoc "${CMAKE_CURRENT_SOURCE_DIR}/${page}.adoc")
	set(html "${CMAKE_CURRENT_BINARY_DIR}/${page}.html")
	add_custom_command(
		OUTPUT "${html}"
		COMMAND "${ASCIIDOCTOR}" ${MANUAL_OPTIONS} -o "${html}" "${adoc}"
		DEPENDS "${adoc}" ${MANUAL_CODE}
		COMMENT "Rendering ${page}.html"
		VERBATIM)
	list(APPEND MANUAL_HTML "${html}")
endforeach()

add_custom_target(colm-manual ALL DEPENDS ${MANUAL_HTML})

# doc_DATA in Makefile.am
if(COLM_INSTALL_COLM)
	install(FILES ${MANUAL_HTML}
		DESTINATION "${CMAKE_INSTALL_DOCDIR}")
endif()
