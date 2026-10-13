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
	3_05_reduction 8_05_contributing
	3_06_send)

# The example programs, with their output (.exp) and input (.in), which the
# pages include, and the arguments they run with (.args). An example can also
# have colm options (.comp), a host program (.host.c or .host.cc), and the
# reducer.h its commit code includes (.reducer.h), which needs .host.cc. A
# file that an example includes is listed too. Its suffix isn't .lm (the pages
# use .lmi), since the manual suite runs every .lm as an example. Every page
# is rebuilt when one of them changes, which is simpler than tracking which
# page includes which.
set(MANUAL_CODE
	code/assign.lm           code/assign.exp           code/assign.in
	code/emb_define.lm       code/emb_define.exp       code/emb_define.comp
	code/emb_export.lm       code/emb_export.exp       code/emb_export.in
	code/emb_export.host.cc
	code/emb_run.lm          code/emb_run.exp          code/emb_run.in
	code/emb_run.comp        code/emb_run.host.c
	code/expr_cast.lm        code/expr_cast.exp        code/expr_cast.in
	code/expr_ops.lm         code/expr_ops.exp
	code/figure_44.lm        code/figure_44.exp
	code/fizzbuzz.lm         code/fizzbuzz.exp
	code/flow_primes.lm      code/flow_primes.exp
	code/hello_world.lm      code/hello_world.exp
	code/hello_world_ext.lm  code/hello_world_ext.exp
	code/iter_bottom_up.lm   code/iter_bottom_up.exp   code/iter_bottom_up.in
	code/iter_builtin.lm     code/iter_builtin.exp     code/iter_builtin.in
	code/iter_rewrite.lm     code/iter_rewrite.exp     code/iter_rewrite.in
	code/iter_tree.lm        code/iter_tree.exp        code/iter_tree.in
	code/iter_user.lm        code/iter_user.exp
	code/lex_basics.lm       code/lex_basics.exp       code/lex_basics.in
	code/lex_choose.lm       code/lex_choose.exp       code/lex_choose.in
	code/lex_comment.lm      code/lex_comment.exp      code/lex_comment.in
	code/lex_heredoc.lm      code/lex_heredoc.exp      code/lex_heredoc.in
	code/lex_regions.lm      code/lex_regions.exp      code/lex_regions.in
	code/lex_rl.lm           code/lex_rl.exp           code/lex_rl.in
	code/lex_skip.lm         code/lex_skip.exp         code/lex_skip.in
	code/list_queue.lm       code/list_queue.exp
	code/map_count.lm        code/map_count.exp        code/map_count.args
	code/mod_include.lm      code/mod_include.exp      code/mod_include.in
	code/mod_list.lmi
	code/mod_namespace.lm    code/mod_namespace.exp    code/mod_namespace.in
	code/mod_nested.lm       code/mod_nested.exp       code/mod_nested.in
	code/nested_scope.lm     code/nested_scope.exp
	code/parse_commit.lm     code/parse_commit.exp     code/parse_commit.in
	code/parse_def.lm        code/parse_def.exp        code/parse_def.in
	code/parse_order.lm      code/parse_order.exp      code/parse_order.in
	code/parse_prec.lm       code/parse_prec.exp
	code/parse_redef.lm      code/parse_redef.exp      code/parse_redef.in
	code/parse_reject.lm     code/parse_reject.exp     code/parse_reject.in
	code/parse_reparse.lm    code/parse_reparse.exp    code/parse_reparse.in
	code/parse_stop.lm       code/parse_stop.exp       code/parse_stop.in
	code/pat_change.lm       code/pat_change.exp
	code/pat_cons.lm         code/pat_cons.exp         code/pat_cons.in
	code/pat_copy.lm         code/pat_copy.exp
	code/pat_make.lm         code/pat_make.exp
	code/pat_match.lm        code/pat_match.exp        code/pat_match.in
	code/pat_require.lm      code/pat_require.exp      code/pat_require.in
	code/pat_switch.lm       code/pat_switch.exp       code/pat_switch.in
	code/poker.lm            code/poker.exp
	code/prog_args.lm        code/prog_args.exp        code/prog_args.args
	code/reduce_attr.lm      code/reduce_attr.exp      code/reduce_attr.in
	code/reduce_calc.lm      code/reduce_calc.exp      code/reduce_calc.in
	code/reduce_first.lm     code/reduce_first.exp
	code/reduce_sum.lm       code/reduce_sum.exp       code/reduce_sum.in
	code/reduce_sum.host.cc  code/reduce_sum.reducer.h
	code/reduce_when.lm      code/reduce_when.exp      code/reduce_when.in
	code/reduce_when.host.cc code/reduce_when.reducer.h
	code/reference.lm        code/reference.exp
	code/scope.lm            code/scope.exp
	code/send_end.lm         code/send_end.exp
	code/send_files.lm       code/send_files.exp
	code/send_include.lm     code/send_include.exp     code/send_include.in
	code/send_out.lm         code/send_out.exp
	code/send_pieces.lm      code/send_pieces.exp
	code/send_tree.lm        code/send_tree.exp
	code/send_trim.lm        code/send_trim.exp
	code/stmt_print.lm       code/stmt_print.exp
	code/struct_route.lm     code/struct_route.exp
	code/type_share.lm       code/type_share.exp
	code/type_str.lm         code/type_str.exp
	code/var_scope.lm        code/var_scope.exp)
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
