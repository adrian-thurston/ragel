/*
 * Copyright 2007-2012 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _COLM_PROGRAM_H
#define _COLM_PROGRAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <colm/pdarun.h>

struct stack_block
{
	tree_t **data;
	int len;
	int offset;
	struct stack_block *next;
};

struct colm_sections
{
	struct lang_el_info *lel_info;
	long num_lang_els;

	struct struct_el_info *sel_info;
	long num_struct_els;

	struct prod_info *prod_info;
	long num_prods;

	struct region_info *region_info;
	long num_regions;

	code_t *root_code;
	long root_code_len;
	long root_frame_id;

	struct frame_info *frame_info;
	long num_frames;

	struct function_info *function_info;
	long num_functions;

	struct pat_cons_info *pat_repl_info;
	long num_patterns;

	struct pat_cons_node *pat_repl_nodes;
	long num_pattern_nodes;

	struct generic_info *generic_info;
	long num_generics;

	long argv_generic_id;

	const char **litdata;
	long *litlen;
	head_t **literals;
	long num_literals;

	CaptureAttr *capture_attr;
	long num_captured_attr;

	struct fsm_tables *fsm_tables;
	struct pda_tables *pda_tables;
	int *start_states;
	int *eof_lel_ids;
	int *parser_lel_ids;
	long num_parsers;

	long global_size;

	long first_non_term_id;

	long integer_id;
	long string_id;
	long any_id;
	long eof_id;
	long no_token_id;
	long global_id;
	long argv_el_id;

	void (*fsm_execute)( struct pda_run *pda_run, struct stream_impl *input_stream );
	void (*send_named_lang_el)( struct colm_program *prg, tree_t **tree,
			struct pda_run *pda_run, struct stream_impl *input_stream );
	void (*init_bindings)( struct pda_run *pda_run );
	void (*pop_binding)( struct pda_run *pda_run, parse_tree_t *tree );

	tree_t **(*host_call)( program_t *prg, long code, tree_t **sp );

	void (*commit_reduce_forward)( program_t *prg, tree_t **root, struct pda_run *pda_run, parse_tree_t *pt );
	long (*commit_union_sz)( int reducer );
	void (*init_need)();
	int (*reducer_need_tok)( program_t *prg, struct pda_run *pda_run, int id );
	int (*reducer_need_ign)( program_t *prg, struct pda_run *pda_run );
};

struct heap_list
{
	struct colm_struct *head;
	struct colm_struct *tail;
};

struct colm_program
{
	long active_realm;

	int argc;
	const char **argv;
	const int *argl;

	unsigned char ctx_dep_parsing;
	struct colm_sections *rtd;
	struct colm_struct *global;
	int induce_exit;
	int exit_status;

	struct pool_alloc kid_pool;
	struct pool_alloc tree_pool;
	struct pool_alloc parse_tree_pool;
	struct pool_alloc head_pool;
	struct pool_alloc location_pool;

	tree_t *true_val;
	tree_t *false_val;

	struct heap_list heap;

	stream_t *stdin_val;
	stream_t *stdout_val;
	stream_t *stderr_val;

	tree_t *error;

	struct run_buf *alloc_run_buf;

	/* Current stack block limits. Changed when crossing block boundaries. */
	tree_t **sb_beg;
	tree_t **sb_end;
	long sb_total;
	struct stack_block *reserve;
	struct stack_block *stack_block;
	tree_t **stack_root;

	/* Returned value for main program and any exported functions. */
	tree_t *return_val;

	void *red_ctx;

	/* This can be extracted for ownership transfer before a program is deleted. */
	const char **stream_fns;
};

#ifdef __cplusplus
}
#endif

#endif /* _COLM_PROGRAM_H */

