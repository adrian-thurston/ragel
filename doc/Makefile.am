ASCIIDOC = @ASCIIDOC@

OUTPUT=html5
OPTIONS=\
	-a icons -a iconsdir=/usr/share/asciidoc/icons \
	-a max-width=800px \
	-a source-highlighter=pygments

OUT_DIR=./output

OUT_FILES = \
	0_00_welcome.html 0_06_scope.html 1_05_variables.html \
	3_00_language_transformation.html 8_00_advanced_topics.html \
	9_00_q_and_a.html 0_01_installing.html 1_00_language_overview.html \
	1_06_iterators.html 3_01_lex.html 8_01_embedding.html \
	0_02_vim_syntax.html 1_01_statements.html 2_00_types.html \
	3_02_parse.html 8_02_modularity.html 0_03_commandline.html \
	1_02_expressions.html 2_01_basic.html 3_03_match.html \
	8_03_performance.html 0_04_hello_world.html 1_03_control_flow.html \
	2_02_list_map_struct_alias.html 3_04_undo.html 8_04_error_handling.html \
	0_05_fizzbuzz.html 1_04_functions.html 2_03_def.html \
	3_05_reduction.html 8_05_contributing.html

CLEANFILES = $(OUT_FILES)

doc_DATA = $(OUT_FILES)

SUFFIXES = .html .adoc

.adoc.html:
	$(ASCIIDOC) -b $(OUTPUT) $(OPTIONS) -o $@ $<
