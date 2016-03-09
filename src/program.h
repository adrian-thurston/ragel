/*
 *  Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef __COLM_PROGRAM_H
#define __COLM_PROGRAM_H

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

#endif
