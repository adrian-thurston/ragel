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
# CMakeLists.txt reads out of configure.ac. Rouge has no lexer for colm, so
# the build loads its own, colm-rouge.rb. colm-links.rb checks the links in
# the rendered book, and logs each one that leads nowhere in it as an error.
# --failure-level=ERROR makes Asciidoctor exit 1 on an error, the check's or
# its own, but not on a warning, such as the one it gives when Rouge, which is
# optional, is missing.
set(MANUAL_OPTIONS
	-b html5
	-a max-width=800px
	-a source-highlighter=rouge
	-r "${CMAKE_CURRENT_SOURCE_DIR}/colm-rouge.rb"
	-r "${CMAKE_CURRENT_SOURCE_DIR}/colm-links.rb"
	--failure-level=ERROR
	-a webfonts!
	-a suite-release=${SUITE_VERSION}
	-a colm-version=${COLM_VERSION}
	-a "colm-pubdate=${COLM_PUBDATE}")

# The manual is one book, colm-manual.adoc, which includes the chapters, in
# the order they are listed here. Only the book is built. CHAPTERS in
# Makefile.am.
set(MANUAL_CHAPTERS
	0_00_welcome 0_01_installing 0_02_vim_syntax
	0_03_commandline 0_04_hello_world 0_05_fizzbuzz
	0_06_scope 0_07_transformation
	1_00_language_overview 1_01_statements 1_02_expressions
	1_03_control_flow 1_04_functions 1_05_variables
	1_06_iterators 2_00_types 2_01_basic
	2_02_list_map_struct_alias 2_03_def
	3_01_lex 3_02_parse 3_03_match 3_04_undo
	3_05_reduction 3_06_send
	8_01_embedding 8_02_modularity 8_04_error_handling
	8_05_contributing
	9_00_q_and_a)
list(TRANSFORM MANUAL_CHAPTERS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
list(TRANSFORM MANUAL_CHAPTERS APPEND ".adoc")

# The example programs, with their output (.exp) and input (.in), which the
# pages include, the arguments they run with (.args), and the exit status they
# must end with (.exit), 0 without one. An example can also have colm options
# (.comp), a host program (.host.c or .host.cc), and the reducer.h its commit
# code includes (.reducer.h), which needs .host.cc. A file that an example
# includes is listed too. Its suffix isn't .lm (the pages use .lmi), since the
# manual suite runs every .lm as an example. The book is rendered again when
# one of them changes.
set(MANUAL_CODE
	code/assign.lm           code/assign.exp           code/assign.in
	code/ctx_reject.lm       code/ctx_reject.exp       code/ctx_reject.in
	code/ctx_typedef.lm      code/ctx_typedef.exp      code/ctx_typedef.in
	code/ctx_undo.lm         code/ctx_undo.exp         code/ctx_undo.in
	code/emb_define.lm       code/emb_define.exp       code/emb_define.comp
	code/emb_export.lm       code/emb_export.exp       code/emb_export.in
	code/emb_export.host.cc
	code/emb_run.lm          code/emb_run.exp          code/emb_run.in
	code/emb_run.comp        code/emb_run.host.c
	code/err_check.lm        code/err_check.exp        code/err_check.in
	code/err_check.exit
	code/err_parts.lm        code/err_parts.exp        code/err_parts.in
	code/err_parts.exit
	code/err_twice.lm        code/err_twice.exp        code/err_twice.in
	code/err_twice.exit
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
	code/py_find.lm          code/py_find.exp          code/py_find.in
	code/py_grammar.lmi
	code/py_parse.lm         code/py_parse.exp         code/py_parse.in
	code/py_rewrite.lm       code/py_rewrite.exp       code/py_rewrite.in
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

set(MANUAL_ADOC "${CMAKE_CURRENT_SOURCE_DIR}/colm-manual.adoc")
set(MANUAL_HTML "${CMAKE_CURRENT_BINARY_DIR}/colm-manual.html")
set(MANUAL_TMP "${CMAKE_CURRENT_BINARY_DIR}/colm-manual.tmp.html")

# Asciidoctor still writes the HTML when --failure-level makes it exit 1, so it
# writes to a temporary file, which is renamed only on success, whatever the
# generator does with a failed command's output. The temporary file keeps the
# .html suffix: Asciidoctor gives the output file's suffix to a link to another
# page. The clean target removes the temporary file that a failed run leaves
# behind, as one of the target's ADDITIONAL_CLEAN_FILES. It isn't declared a
# byproduct: a successful run renames it away, and Ninja renders the book again
# on every build when a byproduct is missing. The book is rendered again when
# the colm lexer or the check of its links changes.
add_custom_command(
	OUTPUT "${MANUAL_HTML}"
	COMMAND "${ASCIIDOCTOR}" ${MANUAL_OPTIONS} -o "${MANUAL_TMP}" "${MANUAL_ADOC}"
	COMMAND "${CMAKE_COMMAND}" -E rename "${MANUAL_TMP}" "${MANUAL_HTML}"
	DEPENDS "${MANUAL_ADOC}" ${MANUAL_CHAPTERS} ${MANUAL_CODE}
		"${CMAKE_CURRENT_SOURCE_DIR}/colm-rouge.rb"
		"${CMAKE_CURRENT_SOURCE_DIR}/colm-links.rb"
	COMMENT "Rendering colm-manual.html"
	VERBATIM)

add_custom_target(colm-manual ALL DEPENDS ${MANUAL_HTML})
set_property(TARGET colm-manual APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${MANUAL_TMP}")

# doc_DATA in Makefile.am
if(COLM_INSTALL_COLM)
	install(FILES ${MANUAL_HTML}
		DESTINATION "${CMAKE_INSTALL_DOCDIR}")
endif()
