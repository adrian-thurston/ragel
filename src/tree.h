/*
 *  Copyright 2010-2012 Adrian Thurston <thurston@complang.org>
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

#ifndef __COLM_TREE_H
#define __COLM_TREE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <colm/colm.h>
#include <colm/type.h>
#include <colm/input.h>
#include <colm/internal.h>

#define COLM_INDENT_OFF -1

typedef unsigned char code_t;
typedef unsigned long word_t;
typedef unsigned long half_t;

struct bindings;
struct function_info;

typedef struct colm_stream stream_t;
typedef struct colm_parser parser_t;
typedef struct colm_list list_t;
typedef struct colm_list_el list_el_t;
typedef struct colm_map map_t;
typedef struct colm_map_el map_el_t;

typedef struct colm_location
{
	const char *name;
	long line;
	long column;
	long byte;
} location_t;

/* Header located just before string data. */
typedef struct colm_data
{
	const char *data; 
	long length;
	struct colm_location *location;
} head_t;

typedef struct colm_kid
{
	/* The tree needs to be first since pointers to kids are used to reference
	 * trees on the stack. A pointer to the word that is a tree_t* is cast to
	 * a kid_t*. */
	struct colm_tree *tree;
	struct colm_kid *next;
	unsigned char flags;
} kid_t;

typedef struct colm_ref
{
	kid_t *kid;
	struct colm_ref *next;
} ref_t;

typedef struct colm_tree tree_t;

struct tree_pair
{
	tree_t *key;
	tree_t *val;
};

typedef struct colm_parse_tree
{
	short id;
	unsigned short flags;

	struct colm_parse_tree *child;
	struct colm_parse_tree *next;
	struct colm_parse_tree *left_ignore;
	struct colm_parse_tree *right_ignore;
	kid_t *shadow;

	/* Parsing algorithm. */
	long state;
	short cause_reduce;

	/* Retry vars. Might be able to unify lower and upper. */
	long retry_region;
	char retry_lower;
	char retry_upper;
} parse_tree_t;

typedef struct colm_pointer
{
	/* Must overlay tree_t. */
	short id;
	unsigned short flags;
	long refs;
	kid_t *child;

	colm_value_t value;
} pointer_t;

typedef struct colm_str
{
	/* Must overlay tree_t. */
	short id;
	unsigned short flags;
	long refs;
	kid_t *child;

	head_t *value;
} str_t;

/*
 * Maps
 */
struct generic_info
{
	long type;

	long el_struct_id;
	long el_offset;

	enum TYPE key_type;
	long key_offset;

	enum TYPE value_type;
	long value_offset;

	long parser_id;
};

enum IterType
{
	IT_Tree = 1,
	IT_RevTree,
	IT_User
};

typedef struct _TreeIter
{
	enum IterType type;
	ref_t root_ref;
	ref_t ref;
	long search_id;
	tree_t **stack_root;
	long arg_size;
	long yield_size;
	long root_size;
} tree_iter_t;

typedef struct colm_generic_iter
{
	enum IterType type;
	ref_t root_ref;
	ref_t ref;
	tree_t **stack_root;
	long arg_size;
	long yield_size;
	long root_size;
	long generic_id;
} generic_iter_t;

/* This must overlay tree iter because some of the same bytecodes are used. */
typedef struct _RevTreeIter
{
	enum IterType type;
	ref_t root_ref;
	ref_t ref;
	long search_id;
	tree_t **stack_root;
	long arg_size;
	long yield_size;
	long root_size;

	/* For detecting a split at the leaf. */
	kid_t *kid_at_yield;
	long children;
} rev_tree_iter_t;

typedef struct colm_user_iter
{
	enum IterType type;
	/* The current item. */
	ref_t ref;
	tree_t **stack_root;
	long arg_size;
	long yield_size;
	long root_size;

	code_t *resume;
	tree_t **frame;
	long search_id;
} user_iter_t;

void colm_tree_upref( tree_t *tree );
void colm_tree_downref( struct colm_program *prg, tree_t **sp, tree_t *tree );
long colm_cmp_tree( struct colm_program *prg, const tree_t *tree1, const tree_t *tree2 );

tree_t *push_right_ignore( struct colm_program *prg, tree_t *push_to, tree_t *right_ignore );
tree_t *push_left_ignore( struct colm_program *prg, tree_t *push_to, tree_t *left_ignore );
tree_t *pop_right_ignore( struct colm_program *prg, tree_t **sp,
		tree_t *pop_from, tree_t **right_ignore );
tree_t *pop_left_ignore( struct colm_program *prg, tree_t **sp,
		tree_t *pop_from, tree_t **left_ignore );
tree_t *tree_left_ignore( struct colm_program *prg, tree_t *tree );
tree_t *tree_right_ignore( struct colm_program *prg, tree_t *tree );
kid_t *tree_left_ignore_kid( struct colm_program *prg, tree_t *tree );
kid_t *tree_right_ignore_kid( struct colm_program *prg, tree_t *tree );
kid_t *tree_child( struct colm_program *prg, const tree_t *tree );
kid_t *tree_attr( struct colm_program *prg, const tree_t *tree );
kid_t *kid_list_concat( kid_t *list1, kid_t *list2 );
kid_t *tree_extract_child( struct colm_program *prg, tree_t *tree );
kid_t *reverse_kid_list( kid_t *kid );

tree_t *colm_construct_pointer( struct colm_program *prg, colm_value_t value );
tree_t *colm_construct_term( struct colm_program *prg, word_t id, head_t *tokdata );
tree_t *colm_construct_tree( struct colm_program *prg, kid_t *kid,
		tree_t **bindings, long pat );
tree_t *colm_construct_object( struct colm_program *prg, kid_t *kid,
		tree_t **bindings, long lang_el_id );
tree_t *colm_construct_token( struct colm_program *prg, tree_t **args, long nargs );

int test_false( struct colm_program *prg, tree_t *tree );
tree_t *make_tree( struct colm_program *prg, tree_t **args, long nargs );
stream_t *open_file( struct colm_program *prg, tree_t *name, tree_t *mode );
stream_t *colm_stream_open_file( struct colm_program *prg, tree_t *name, tree_t *mode );
stream_t *colm_stream_open_fd( struct colm_program *prg, char *name, long fd );
kid_t *copy_ignore_list( struct colm_program *prg, kid_t *ignore_header );
kid_t *copy_kid_list( struct colm_program *prg, kid_t *kid_list );
void colm_stream_free( struct colm_program *prg, stream_t *s );
tree_t *colm_copy_tree( struct colm_program *prg, tree_t *tree,
		kid_t *old_next_down, kid_t **new_next_down );

colm_value_t colm_get_pointer_val( tree_t *pointer );
tree_t *colm_tree_get_field( tree_t *tree, word_t field );
tree_t *get_field_split( struct colm_program *prg, tree_t *tree, word_t field );
tree_t *get_rhs_el( struct colm_program *prg, tree_t *lhs, long position );
kid_t *get_rhs_el_kid( struct colm_program *prg, tree_t *lhs, long position );
parse_tree_t *get_rhs_parse_tree( struct colm_program *prg,
		parse_tree_t *lhs, long position );
void colm_tree_set_field( struct colm_program *prg, tree_t *tree, long field, tree_t *value );

void set_triter_cur( struct colm_program *prg, tree_iter_t *iter, tree_t *tree );
void set_uiter_cur( struct colm_program *prg, user_iter_t *uiter, tree_t *tree );
void ref_set_value( struct colm_program *prg, tree_t **sp, ref_t *ref, tree_t *v );
tree_t *tree_search( struct colm_program *prg, tree_t *tree, long id );

int match_pattern( tree_t **bindings, struct colm_program *prg,
		long pat, kid_t *kid, int check_next );
tree_t *tree_iter_deref_cur( tree_iter_t *iter );

/* For making references of attributes. */
kid_t *get_field_kid( tree_t *tree, word_t field );

tree_t *copy_real_tree( struct colm_program *prg, tree_t *tree,
		kid_t *old_next_down, kid_t **new_next_down );
void split_iter_cur( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );
tree_t *set_list_mem( list_t *list, half_t field, tree_t *value );

void list_push_tail( struct colm_program *prg, list_t *list, tree_t *val );
void list_push_head( struct colm_program *prg, list_t *list, tree_t *val );
tree_t *list_remove_end( struct colm_program *prg, list_t *list );
tree_t *list_remove_head( struct colm_program *prg, list_t *list );
tree_t *get_list_mem_split( struct colm_program *prg, list_t *list, word_t field );
tree_t *get_parser_mem( parser_t *parser, word_t field );

tree_t *tree_iter_advance( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );
tree_t *tree_iter_next_child( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );
tree_t *tree_rev_iter_prev_child( struct colm_program *prg, tree_t ***psp, rev_tree_iter_t *iter );
tree_t *tree_iter_next_repeat( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );
tree_t *tree_iter_prev_repeat( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );

/* An automatically grown buffer for collecting tokens. Always reuses space;
 * never down resizes. */
typedef struct _StrCollect
{
	char *data;
	int allocated;
	int length;
} StrCollect;

void init_str_collect( StrCollect *collect );
void str_collect_destroy( StrCollect *collect );
void str_collect_append( StrCollect *collect, const char *data, long len );
void str_collect_clear( StrCollect *collect );
tree_t *tree_trim( struct colm_program *prg, tree_t **sp, tree_t *tree );

void print_tree_collect( struct colm_program *prg, tree_t **sp,
		StrCollect *collect, tree_t *tree, int trim );

void print_tree_collect_a( struct colm_program *prg, tree_t **sp,
		StrCollect *collect, tree_t *tree, int trim );

void print_tree_file( struct colm_program *prg, tree_t **sp,
		struct stream_impl *impl, tree_t *tree, int trim );
void print_xml_stdout( struct colm_program *prg, tree_t **sp,
		struct stream_impl *impl, tree_t *tree, int comm_attr, int trim );

/*
 * Iterators.
 */

user_iter_t *colm_uiter_create( struct colm_program *prg, tree_t ***psp,
		struct function_info *fi, long search_id );
void uiter_init( struct colm_program *prg, tree_t **sp, user_iter_t *uiter, 
		struct function_info *fi, int revert_on );

void colm_init_tree_iter( tree_iter_t *tree_iter, tree_t **stack_root,
		long arg_size, long root_size, const ref_t *root_ref, int search_id );
void colm_init_rev_tree_iter( rev_tree_iter_t *rev_triter, tree_t **stack_root,
		long arg_size, long root_size, const ref_t *root_ref, int search_id, int children );
void colm_init_user_iter( user_iter_t *user_iter, tree_t **stack_root, long root_size,
		long arg_size, long search_id );

void colm_tree_iter_destroy( struct colm_program *prg,
		tree_t ***psp, tree_iter_t *iter );

void colm_rev_tree_iter_destroy( struct colm_program *prg,
		tree_t ***psp, rev_tree_iter_t *iter );

void colm_uiter_destroy( struct colm_program *prg, tree_t ***psp, user_iter_t *uiter );
void colm_uiter_unwind( struct colm_program *prg, tree_t ***psp, user_iter_t *uiter );

tree_t *cast_tree( struct colm_program *prg, int lang_el_id, tree_t *tree );
struct stream_impl *stream_to_impl( stream_t *ptr );

void colm_init_list_iter( generic_iter_t *list_iter, tree_t **stack_root,
		long arg_size, long root_size, const ref_t *root_ref, int generic_id );
void colm_list_iter_destroy( struct colm_program *prg,
		tree_t ***psp, generic_iter_t *iter );

tree_t *colm_list_iter_advance( struct colm_program *prg,
		tree_t ***psp, generic_iter_t *iter );
tree_t *colm_rev_list_iter_advance( struct colm_program *prg,
		tree_t ***psp, generic_iter_t *iter );

tree_t *colm_list_iter_deref_cur( struct colm_program *prg, generic_iter_t *iter );
void colm_list_append( struct colm_list *list, struct colm_list_el *new_el );
void colm_list_prepend( struct colm_list *list, struct colm_list_el *new_el );

void colm_vlist_append( struct colm_program *prg, list_t *list, value_t value );
void colm_vlist_prepend( struct colm_program *prg, list_t *list, value_t value );
value_t colm_vlist_detach_head( struct colm_program *prg, list_t *list );
value_t colm_vlist_detach_tail( struct colm_program *prg, list_t *list );

value_t colm_viter_deref_cur( struct colm_program *prg, generic_iter_t *iter );

str_t *string_prefix( program_t *prg, str_t *str, long len );
str_t *string_suffix( program_t *prg, str_t *str, long pos );
head_t *string_alloc_full( struct colm_program *prg, const char *data, long length );
tree_t *construct_string( struct colm_program *prg, head_t *s );

#if defined(__cplusplus)
}
#endif

#endif

