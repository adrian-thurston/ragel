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

#ifndef __COLM_PDARUN_H
#define __COLM_PDARUN_H

#include <colm/input.h>
#include <colm/defs.h>
#include <colm/tree.h>
#include <colm/struct.h>

#ifdef __cplusplus
extern "C" {
#endif

struct colm_program;

#define MARK_SLOTS 32

struct fsm_tables
{
	long *actions;
	long *key_offsets;
	char *trans_keys;
	long *single_lengths;
	long *range_lengths;
	long *index_offsets;
	long *transTargsWI;
	long *transActionsWI;
	long *to_state_actions;
	long *from_state_actions;
	long *eof_actions;
	long *eof_targs;
	long *entry_by_region;

	long num_states;
	long num_actions;
	long num_trans_keys;
	long num_single_lengths;
	long num_range_lengths;
	long num_index_offsets;
	long numTransTargsWI;
	long numTransActionsWI;
	long num_regions;

	long start_state;
	long first_final;
	long error_state;

	struct GenAction **action_switch;
	long num_action_switch;
};

void undo_stream_pull( struct stream_impl *input_stream, const char *data, long length );

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8 
	#error "SIZEOF_LONG contained an unexpected value"
#endif

struct colm_execution;

struct rt_code_vect
{
	code_t *data;
	long tab_len;
	long alloc_len;

	/* FIXME: leak when freed. */
};

void list_add_after( list_t *list, list_el_t *prev_el, list_el_t *new_el );
void list_add_before( list_t *list, list_el_t *next_el, list_el_t *new_el );

void list_prepend( list_t *list, list_el_t *new_el );
void list_append( list_t *list, list_el_t *new_el );

list_el_t *list_detach( list_t *list, list_el_t *el );
list_el_t *list_detach_first(list_t *list );
list_el_t *list_detach_last(list_t *list );

long list_length(list_t *list);

struct function_info
{
	long frame_id;
	long arg_size;
	long frame_size;
};

/*
 * Program Data.
 */

struct pat_cons_info
{
	long offset;
	long num_bindings;
};

struct pat_cons_node
{
	long id;
	long prod_num;
	long next;
	long child;
	long bind_id;
	const char *data;
	long length;
	long left_ignore;
	long right_ignore;

	/* Just match nonterminal, don't go inside. */
	unsigned char stop;
};

/* FIXME: should have a descriptor for object types to give the length. */

struct lang_el_info
{
	const char *name;
	const char *xml_tag;
	unsigned char repeat;
	unsigned char list;
	unsigned char literal;
	unsigned char ignore;

	long frame_id;

	long object_type_id;
	long ofi_offset;
	long object_length;

	long term_dup_id;
	long mark_id;
	long capture_attr;
	long num_capture_attr;
};

struct struct_el_info
{
	long size;
	short *trees;
	long trees_len;
};

struct prod_info
{
	unsigned long lhs_id;
	short prod_num;
	long length;
	const char *name;
	long frame_id;
	unsigned char lhs_upref;
	unsigned char *copy;
	long copy_len;
};

/* Must match the LocalType enum. */
#define LI_Tree 1
#define LI_Iter 2
#define LI_RevIter 3
#define LI_UserIter 4

struct local_info
{
	char type;
	short offset;
};

struct frame_info
{
	const char *name;
	code_t *codeWV;
	long codeLenWV;
	code_t *codeWC;
	long codeLenWC;
	struct local_info *locals;
	long locals_len;
	long arg_size;
	long frame_size;
	char ret_tree;
};

struct region_info
{
	long default_token;
	long eof_frame_id;
	int ci_lel_id;
};

typedef struct _CaptureAttr
{
	long mark_enter;
	long mark_leave;
	long offset;
} CaptureAttr;

struct pda_tables
{
	/* Parser table data. */
	int *indicies;
	int *owners;
	int *keys;
	unsigned int *offsets;
	unsigned int *targs;
	unsigned int *act_inds;
	unsigned int *actions;
	int *commit_len;
	int *token_region_inds;
	int *token_regions;
	int *token_pre_regions;

	int num_indicies;
	int num_keys;
	int num_states;
	int num_targs;
	int num_act_inds;
	int num_actions;
	int num_commit_len;
	int num_region_items;
	int num_pre_region_items;
};

struct pool_block
{
	void *data;
	struct pool_block *next;
};

struct pool_item
{
	struct pool_item *next;
};

struct pool_alloc
{
	struct pool_block *head;
	long nextel;
	struct pool_item *pool;
	int sizeofT;
};

struct pda_run
{
	/*
	 * Scanning.
	 */
	struct fsm_tables *fsm_tables;

	struct run_buf *consume_buf;

	long region, pre_region;
	long fsm_cs, next_cs, act;
	char *start;
	char *tokstart;
	long tokend;
	long toklen;
	char *p, *pe;

	/* Bits. */
	char eof;
	char return_result;
	char skip_toklen;

	char *mark[MARK_SLOTS];
	long matched_token;

	/*
	 * Parsing
	 */
	int num_retry;
	parse_tree_t *stack_top;
	ref_t *token_list;
	int pda_cs;
	int next_region_ind;

	struct pda_tables *pda_tables;
	int parser_id;

	/* Reused. */
	struct rt_code_vect rcode_collect;
	struct rt_code_vect reverse_code;

	int stop_parsing;
	long stop_target;

	parse_tree_t *accum_ignore;

	kid_t *bt_point;

	struct bindings *bindings;

	int revert_on;

	struct colm_struct *context;

	int stop;
	int parse_error;

	long steps;
	long target_steps;


	/* The shift count simply tracks the number of shifts that have happend.
	 * The commit shift count is the shift count when the last commit occurred.
	 * If we back up to this number of shifts then we decide we cannot proceed.
	 * The commit shift count is initialized to -1. */
	long shift_count;
	long commit_shift_count;

	int on_deck;

	/*
	 * Data we added when refactoring the parsing engine into a coroutine.
	 */

	parse_tree_t *parse_input;
	struct frame_info *fi;
	int reduction;
	parse_tree_t *red_lel;
	int cur_state;
	parse_tree_t *lel;
	int trigger_undo;

	int token_id;
	head_t *tokdata;
	int frame_id;
	int next;
	parse_tree_t *undo_lel;

	int check_next;
	int check_stop;

	/* The lhs is sometimes saved before reduction actions in case it is
	 * replaced and we need to restore it on backtracking */
	tree_t *parsed;

	int reject;

	/* Instruction pointer to use when we stop parsing and execute code. */
	code_t *code;

	int rc_block_count;

	tree_t *parse_error_text;

	/* Zero indicates parsing proper. Nonzero is the reducer id. */
	int reducer;

	parse_tree_t *last_final;

	struct pool_alloc *parse_tree_pool;
	struct pool_alloc local_pool;

	/* Disregard any alternate parse paths, just go right to failure. */
	int fail_parsing;
};

void colm_pda_init( struct colm_program *prg, struct pda_run *pda_run,
		struct pda_tables *tables, int parser_id, long stop_target,
		int revert_on, struct colm_struct *context, int reducer );

void colm_pda_clear( struct colm_program *prg, struct colm_tree **sp,
		struct pda_run *pda_run );

void colm_rt_code_vect_replace( struct rt_code_vect *vect, long pos,
		const code_t *val, long len );
void colm_rt_code_vect_empty( struct rt_code_vect *vect );
void colm_rt_code_vect_remove( struct rt_code_vect *vect, long pos, long len );

void init_rt_code_vect( struct rt_code_vect *code_vect );

inline static void append_code_val( struct rt_code_vect *vect, const code_t val );
inline static void append_code_vect( struct rt_code_vect *vect, const code_t *val, long len );
inline static void append_half( struct rt_code_vect *vect, half_t half );
inline static void append_word( struct rt_code_vect *vect, word_t word );

inline static void append_code_vect( struct rt_code_vect *vect, const code_t *val, long len )
{
	colm_rt_code_vect_replace( vect, vect->tab_len, val, len );
}

inline static void append_code_val( struct rt_code_vect *vect, const code_t val )
{
	colm_rt_code_vect_replace( vect, vect->tab_len, &val, 1 );
}

inline static void append_half( struct rt_code_vect *vect, half_t half )
{
	/* not optimal. */
	append_code_val( vect, half & 0xff );
	append_code_val( vect, (half>>8) & 0xff );
}

inline static void append_word( struct rt_code_vect *vect, word_t word )
{
	/* not optimal. */
	append_code_val( vect, word & 0xff );
	append_code_val( vect, (word>>8) & 0xff );
	append_code_val( vect, (word>>16) & 0xff );
	append_code_val( vect, (word>>24) & 0xff );
	#if SIZEOF_LONG == 8
	append_code_val( vect, (word>>32) & 0xff );
	append_code_val( vect, (word>>40) & 0xff );
	append_code_val( vect, (word>>48) & 0xff );
	append_code_val( vect, (word>>56) & 0xff );
	#endif
}

void colm_increment_steps( struct pda_run *pda_run );
void colm_decrement_steps( struct pda_run *pda_run );

void colm_clear_stream_impl( struct colm_program *prg, tree_t **sp, struct stream_impl *input_stream );
void colm_clear_source_stream( struct colm_program *prg, tree_t **sp, struct stream_impl *source_stream );

#define PCR_START         1
#define PCR_DONE          2
#define PCR_REDUCTION     3
#define PCR_GENERATION    4
#define PCR_PRE_EOF       5
#define PCR_REVERSE       6

head_t *colm_stream_pull( struct colm_program *prg, struct colm_tree **sp,
		struct pda_run *pda_run, struct stream_impl *is, long length );
head_t *colm_string_alloc_pointer( struct colm_program *prg, const char *data, long length );

void colm_stream_push_text( struct stream_impl *input_stream, const char *data, long length );
void colm_stream_push_tree( struct stream_impl *input_stream, tree_t *tree, int ignore );
void colm_stream_push_stream( struct stream_impl *input_stream, tree_t *tree );
void colm_undo_stream_push( struct colm_program *prg, tree_t **sp,
		struct stream_impl *input_stream, long length );

kid_t *make_token_with_data( struct colm_program *prg, struct pda_run *pda_run,
		struct stream_impl *input_stream, int id, head_t *tokdata );

long colm_parse_loop( struct colm_program *prg, tree_t **sp, struct pda_run *pda_run, 
		struct stream_impl *input_stream, long entry );

long colm_parse_frag( struct colm_program *prg, tree_t **sp, struct pda_run *pda_run,
		stream_t *input, long stop_id, long entry );
long colm_parse_finish( tree_t **result, struct colm_program *prg, tree_t **sp,
		struct pda_run *pda_run, stream_t *input , int revert_on, long entry );
long colm_parse_undo_frag( struct colm_program *prg, tree_t **sp, struct pda_run *pda_run,
		stream_t *input, long steps, long entry );

void commit_clear_kid_list( program_t *prg, tree_t **sp, kid_t *kid );
void commit_clear_parse_tree( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, parse_tree_t *pt );
void commit_reduce( program_t *prg, tree_t **root,
		struct pda_run *pda_run );

#ifdef __cplusplus
}
#endif

#endif
