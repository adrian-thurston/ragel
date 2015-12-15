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

#include <colm/pdarun.h>
#include <colm/tree.h>
#include <colm/pool.h>
#include <colm/bytecode.h>
#include <colm/debug.h>
#include <colm/map.h>
#include <colm/struct.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#define true 1
#define false 0

#define BUFFER_INITIAL_SIZE 4096

kid_t *alloc_attrs( program_t *prg, long length )
{
	kid_t *cur = 0;
	long i;
	for ( i = 0; i < length; i++ ) {
		kid_t *next = cur;
		cur = kid_allocate( prg );
		cur->next = next;
	}
	return cur;
}

void free_attrs( program_t *prg, kid_t *attrs )
{
	kid_t *cur = attrs;
	while ( cur != 0 ) {
		kid_t *next = cur->next;
		kid_free( prg, cur );
		cur = next;
	}
}

void free_kid_list( program_t *prg, kid_t *kid )
{
	while ( kid != 0 ) {
		kid_t *next = kid->next;
		kid_free( prg, kid );
		kid = next;
	}
}

static void colm_tree_set_attr( tree_t *tree, long pos, tree_t *val )
{
	long i;
	kid_t *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	for ( i = 0; i < pos; i++ )
		kid = kid->next;
	kid->tree = val;
}

tree_t *colm_get_attr( tree_t *tree, long pos )
{
	long i;
	kid_t *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	for ( i = 0; i < pos; i++ )
		kid = kid->next;
	return kid->tree;
}


tree_t *colm_get_repeat_next( tree_t *tree )
{
	kid_t *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	return kid->next->tree;
}

tree_t *colm_get_repeat_val( tree_t *tree )
{
	kid_t *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;
	
	return kid->tree;
}

int colm_repeat_end( tree_t *tree )
{
	kid_t *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	return kid == 0;
}

int colm_list_last( tree_t *tree )
{
	kid_t *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	return kid->next == 0;
}

kid_t *get_attr_kid( tree_t *tree, long pos )
{
	long i;
	kid_t *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	for ( i = 0; i < pos; i++ )
		kid = kid->next;
	return kid;
}

kid_t *kid_list_concat( kid_t *list1, kid_t *list2 )
{
	if ( list1 == 0 )
		return list2;
	else if ( list2 == 0 )
		return list1;

	kid_t *dest = list1;
	while ( dest->next != 0 )
		dest = dest->next;
	dest->next = list2;
	return list1;
}

tree_t *colm_construct_pointer( program_t *prg, value_t value )
{
	pointer_t *pointer = (pointer_t*) tree_allocate( prg );
	pointer->id = LEL_ID_PTR;
	pointer->value = value;
	
	return (tree_t*)pointer;
}

value_t colm_get_pointer_val( tree_t *ptr )
{
	return ((pointer_t*)ptr)->value;
}


tree_t *colm_construct_term( program_t *prg, word_t id, head_t *tokdata )
{
	struct lang_el_info *lel_info = prg->rtd->lel_info;

	tree_t *tree = tree_allocate( prg );
	tree->id = id;
	tree->refs = 0;
	tree->tokdata = tokdata;

	int object_length = lel_info[tree->id].object_length;
	tree->child = alloc_attrs( prg, object_length );

	return tree;
}


kid_t *construct_kid( program_t *prg, tree_t **bindings, kid_t *prev, long pat );

static kid_t *construct_ignore_list( program_t *prg, long ignore_ind )
{
	struct pat_cons_node *nodes = prg->rtd->pat_repl_nodes;

	kid_t *first = 0, *last = 0;
	while ( ignore_ind >= 0 ) {
		head_t *ignore_data = colm_string_alloc_pointer( prg, nodes[ignore_ind].data,
				nodes[ignore_ind].length );

		tree_t *ign_tree = tree_allocate( prg );
		ign_tree->refs = 1;
		ign_tree->id = nodes[ignore_ind].id;
		ign_tree->tokdata = ignore_data;

		kid_t *ign_kid = kid_allocate( prg );
		ign_kid->tree = ign_tree;
		ign_kid->next = 0;

		if ( last == 0 )
			first = ign_kid;
		else
			last->next = ign_kid;

		ignore_ind = nodes[ignore_ind].next;
		last = ign_kid;
	}

	return first;
}

static kid_t *construct_left_ignore_list( program_t *prg, long pat )
{
	struct pat_cons_node *nodes = prg->rtd->pat_repl_nodes;
	return construct_ignore_list( prg, nodes[pat].left_ignore );
}

static kid_t *construct_right_ignore_list( program_t *prg, long pat )
{
	struct pat_cons_node *nodes = prg->rtd->pat_repl_nodes;
	return construct_ignore_list( prg, nodes[pat].right_ignore );
}

static void ins_left_ignore( program_t *prg, tree_t *tree, tree_t *ignore_list )
{
	assert( ! (tree->flags & AF_LEFT_IGNORE) );

	/* Allocate. */
	kid_t *kid = kid_allocate( prg );
	kid->tree = ignore_list;
	colm_tree_upref( ignore_list );

	/* Attach it. */
	kid->next = tree->child;
	tree->child = kid;

	tree->flags |= AF_LEFT_IGNORE;
}

static void ins_right_ignore( program_t *prg, tree_t *tree, tree_t *ignore_list )
{
	assert( ! (tree->flags & AF_RIGHT_IGNORE) );

	/* Insert an ignore head in the child list. */
	kid_t *kid = kid_allocate( prg );
	kid->tree = ignore_list;
	colm_tree_upref( ignore_list );

	/* Attach it. */
	if ( tree->flags & AF_LEFT_IGNORE ) {
		kid->next = tree->child->next;
		tree->child->next = kid;
	}
	else {
		kid->next = tree->child;
		tree->child = kid;
	}

	tree->flags |= AF_RIGHT_IGNORE;
}

tree_t *push_right_ignore( program_t *prg, tree_t *push_to, tree_t *right_ignore )
{
	/* About to alter the data tree. Split first. */
	push_to = split_tree( prg, push_to );

	if ( push_to->flags & AF_RIGHT_IGNORE ) {
		/* The previous token already has a right ignore. Merge by
		 * attaching it as a left ignore of the new list. */
		kid_t *cur_ignore = tree_right_ignore_kid( prg, push_to );
		ins_left_ignore( prg, right_ignore, cur_ignore->tree );

		/* Replace the current ignore. Safe to access refs here because we just
		 * upreffed it in insLeftIgnore. */
		cur_ignore->tree->refs -= 1;
		cur_ignore->tree = right_ignore;
		colm_tree_upref( right_ignore );
	}
	else {
		/* Attach The ignore list. */
		ins_right_ignore( prg, push_to, right_ignore );
	}

	return push_to;
}

tree_t *push_left_ignore( program_t *prg, tree_t *push_to, tree_t *left_ignore )
{
	push_to = split_tree( prg, push_to );

	/* Attach as left ignore to the token we are sending. */
	if ( push_to->flags & AF_LEFT_IGNORE ) {
		/* The token already has a left-ignore. Merge by attaching it as a
		 * right ignore of the new list. */
		kid_t *cur_ignore = tree_left_ignore_kid( prg, push_to );
		ins_right_ignore( prg, left_ignore, cur_ignore->tree );

		/* Replace the current ignore. Safe to upref here because we just
		 * upreffed it in insRightIgnore. */
		cur_ignore->tree->refs -= 1;
		cur_ignore->tree = left_ignore;
		colm_tree_upref( left_ignore );
	}
	else {
		/* Attach the ignore list. */
		ins_left_ignore( prg, push_to, left_ignore );
	}

	return push_to;
}

static void rem_left_ignore( program_t *prg, tree_t **sp, tree_t *tree )
{
	assert( tree->flags & AF_LEFT_IGNORE );

	kid_t *next = tree->child->next;
	colm_tree_downref( prg, sp, tree->child->tree );
	kid_free( prg, tree->child );
	tree->child = next;

	tree->flags &= ~AF_LEFT_IGNORE;
}

static void rem_right_ignore( program_t *prg, tree_t **sp, tree_t *tree )
{
	assert( tree->flags & AF_RIGHT_IGNORE );

	if ( tree->flags & AF_LEFT_IGNORE ) {
		kid_t *next = tree->child->next->next;
		colm_tree_downref( prg, sp, tree->child->next->tree );
		kid_free( prg, tree->child->next );
		tree->child->next = next;
	}
	else {
		kid_t *next = tree->child->next;
		colm_tree_downref( prg, sp, tree->child->tree );
		kid_free( prg, tree->child );
		tree->child = next;
	}

	tree->flags &= ~AF_RIGHT_IGNORE;
}

tree_t *pop_right_ignore( program_t *prg, tree_t **sp, tree_t *pop_from, tree_t **right_ignore )
{
	/* Modifying the tree we are detaching from. */
	pop_from = split_tree( prg, pop_from );

	kid_t *ri_kid = tree_right_ignore_kid( prg, pop_from );

	/* If the right ignore has a left ignore, then that was the original
	 * right ignore. */
	kid_t *li = tree_left_ignore_kid( prg, ri_kid->tree );
	if ( li != 0 ) {
		colm_tree_upref( li->tree );
		rem_left_ignore( prg, sp, ri_kid->tree );
		*right_ignore = ri_kid->tree;
		colm_tree_upref( *right_ignore );
		ri_kid->tree = li->tree;
	}
	else  {
		*right_ignore = ri_kid->tree;
		colm_tree_upref( *right_ignore );
		rem_right_ignore( prg, sp, pop_from );
	}

	return pop_from;
}

tree_t *pop_left_ignore( program_t *prg, tree_t **sp, tree_t *pop_from, tree_t **left_ignore )
{
	/* Modifying, make the write safe. */
	pop_from = split_tree( prg, pop_from );

	kid_t *li_kid = tree_left_ignore_kid( prg, pop_from );

	/* If the left ignore has a right ignore, then that was the original
	 * left ignore. */
	kid_t *ri = tree_right_ignore_kid( prg, li_kid->tree );
	if ( ri != 0 ) {
		colm_tree_upref( ri->tree );
		rem_right_ignore( prg, sp, li_kid->tree );
		*left_ignore = li_kid->tree;
		colm_tree_upref( *left_ignore );
		li_kid->tree = ri->tree;
	}
	else {
		*left_ignore = li_kid->tree;
		colm_tree_upref( *left_ignore );
		rem_left_ignore( prg, sp, pop_from );
	}

	return pop_from;
}

tree_t *colm_construct_object( program_t *prg, kid_t *kid, tree_t **bindings, long lang_el_id )
{
	struct lang_el_info *lel_info = prg->rtd->lel_info;
	tree_t *tree = 0;

	tree = tree_allocate( prg );
	tree->id = lang_el_id;
	tree->refs = 1;
	tree->tokdata = 0;
	tree->prod_num = 0;

	int object_length = lel_info[tree->id].object_length;

	kid_t *attrs = alloc_attrs( prg, object_length );
	kid_t *child = 0;

	tree->child = kid_list_concat( attrs, child );

	return tree;
}

/* Returns an uprefed tree. Saves us having to downref and bindings to zero to
 * return a zero-ref tree. */
tree_t *colm_construct_tree( program_t *prg, kid_t *kid, tree_t **bindings, long pat )
{
	struct pat_cons_node *nodes = prg->rtd->pat_repl_nodes;
	struct lang_el_info *lel_info = prg->rtd->lel_info;
	tree_t *tree = 0;

	if ( nodes[pat].bind_id > 0 ) {
		/* All bindings have been uprefed. */
		tree = bindings[nodes[pat].bind_id];

		long ignore = nodes[pat].left_ignore;
		tree_t *left_ignore = 0;
		if ( ignore >= 0 ) {
			kid_t *ignore = construct_left_ignore_list( prg, pat );

			left_ignore = tree_allocate( prg );
			left_ignore->id = LEL_ID_IGNORE;
			left_ignore->child = ignore;

			tree = push_left_ignore( prg, tree, left_ignore );
		}

		ignore = nodes[pat].right_ignore;
		tree_t *right_ignore = 0;
		if ( ignore >= 0 ) {
			kid_t *ignore = construct_right_ignore_list( prg, pat );

			right_ignore = tree_allocate( prg );
			right_ignore->id = LEL_ID_IGNORE;
			right_ignore->child = ignore;

			tree = push_right_ignore( prg, tree, right_ignore );
		}
	}
	else {
		tree = tree_allocate( prg );
		tree->id = nodes[pat].id;
		tree->refs = 1;
		tree->tokdata = nodes[pat].length == 0 ? 0 :
				colm_string_alloc_pointer( prg, 
				nodes[pat].data, nodes[pat].length );
		tree->prod_num = nodes[pat].prod_num;

		int object_length = lel_info[tree->id].object_length;

		kid_t *attrs = alloc_attrs( prg, object_length );
		kid_t *child = construct_kid( prg, bindings,
				0, nodes[pat].child );

		tree->child = kid_list_concat( attrs, child );

		/* Right first, then left. */
		kid_t *ignore = construct_right_ignore_list( prg, pat );
		if ( ignore != 0 ) {
			tree_t *ignore_list = tree_allocate( prg );
			ignore_list->id = LEL_ID_IGNORE;
			ignore_list->refs = 1;
			ignore_list->child = ignore;

			kid_t *ignore_head = kid_allocate( prg );
			ignore_head->tree = ignore_list;
			ignore_head->next = tree->child;
			tree->child = ignore_head;

			tree->flags |= AF_RIGHT_IGNORE;
		}

		ignore = construct_left_ignore_list( prg, pat );
		if ( ignore != 0 ) {
			tree_t *ignore_list = tree_allocate( prg );
			ignore_list->id = LEL_ID_IGNORE;
			ignore_list->refs = 1;
			ignore_list->child = ignore;

			kid_t *ignore_head = kid_allocate( prg );
			ignore_head->tree = ignore_list;
			ignore_head->next = tree->child;
			tree->child = ignore_head;

			tree->flags |= AF_LEFT_IGNORE;
		}

		int i;
		for ( i = 0; i < lel_info[tree->id].num_capture_attr; i++ ) {
			long ci = pat+1+i;
			CaptureAttr *ca = prg->rtd->capture_attr + lel_info[tree->id].capture_attr + i;
			tree_t *attr = tree_allocate( prg );
			attr->id = nodes[ci].id;
			attr->refs = 1;
			attr->tokdata = nodes[ci].length == 0 ? 0 :
					colm_string_alloc_pointer( prg, 
					nodes[ci].data, nodes[ci].length );

			colm_tree_set_attr( tree, ca->offset, attr );
		}
	}

	return tree;
}

kid_t *construct_kid( program_t *prg, tree_t **bindings, kid_t *prev, long pat )
{
	struct pat_cons_node *nodes = prg->rtd->pat_repl_nodes;
	kid_t *kid = 0;

	if ( pat != -1 ) {
		kid = kid_allocate( prg );
		kid->tree = colm_construct_tree( prg, kid, bindings, pat );

		/* Recurse down next. */
		kid_t *next = construct_kid( prg, bindings,
				kid, nodes[pat].next );

		kid->next = next;
	}

	return kid;
}

tree_t *colm_construct_token( program_t *prg, tree_t **args, long nargs )
{
	value_t id_int = (value_t)args[0];
	str_t *text_str = (str_t*)args[1];

	long id = (long)id_int;
	head_t *tokdata = string_copy( prg, text_str->value );

	struct lang_el_info *lel_info = prg->rtd->lel_info;
	tree_t *tree;

	if ( lel_info[id].ignore ) {
		tree = tree_allocate( prg );
		tree->refs = 1;
		tree->id = id;
		tree->tokdata = tokdata;
	}
	else {
		long object_length = lel_info[id].object_length;
		assert( nargs-2 <= object_length );

		kid_t *attrs = alloc_attrs( prg, object_length );

		tree = tree_allocate( prg );
		tree->id = id;
		tree->refs = 1;
		tree->tokdata = tokdata;

		tree->child = attrs;

		long i;
		for ( i = 2; i < nargs; i++ ) {
			colm_tree_set_attr( tree, i-2, args[i] );
			colm_tree_upref( colm_get_attr( tree, i-2 ) );
		}
	}
	return tree;
}

tree_t *cast_tree( program_t *prg, int lang_el_id, tree_t *tree )
{
	struct lang_el_info *lel_info = prg->rtd->lel_info;

	/* Need to keep a lookout for next down. If 
	 * copying it, return the copy. */
	tree_t *new_tree = tree_allocate( prg );

	new_tree->id = lang_el_id;
	new_tree->tokdata = string_copy( prg, tree->tokdata );

	/* Invalidate the production number. */
	new_tree->prod_num = -1;

	/* Copy the child list. Start with ignores, then the list. */
	kid_t *child = tree->child, *last = 0;

	/* Flags we are interested in. */
	new_tree->flags |= tree->flags & ( AF_LEFT_IGNORE | AF_RIGHT_IGNORE );

	int ignores = 0;
	if ( tree->flags & AF_LEFT_IGNORE )
		ignores += 1;
	if ( tree->flags & AF_RIGHT_IGNORE )
		ignores += 1;

	/* Igores. */
	while ( ignores-- > 0 ) {
		kid_t *new_kid = kid_allocate( prg );

		new_kid->tree = child->tree;
		new_kid->next = 0;
		new_kid->tree->refs += 1;

		/* Store the first child. */
		if ( last == 0 )
			new_tree->child = new_kid;
		else
			last->next = new_kid;

		child = child->next;
		last = new_kid;
	}

	/* Skip over the source's attributes. */
	int object_length = lel_info[tree->id].object_length;
	while ( object_length-- > 0 )
		child = child->next;

	/* Allocate the target type's kids. */
	object_length = lel_info[lang_el_id].object_length;
	while ( object_length-- > 0 ) {
		kid_t *new_kid = kid_allocate( prg );

		new_kid->tree = 0;
		new_kid->next = 0;

		/* Store the first child. */
		if ( last == 0 )
			new_tree->child = new_kid;
		else
			last->next = new_kid;

		last = new_kid;
	}
	
	/* Copy the source's children. */
	while ( child != 0 ) {
		kid_t *new_kid = kid_allocate( prg );

		new_kid->tree = child->tree;
		new_kid->next = 0;
		new_kid->tree->refs += 1;

		/* Store the first child. */
		if ( last == 0 )
			new_tree->child = new_kid;
		else
			last->next = new_kid;

		child = child->next;
		last = new_kid;
	}
	
	return new_tree;
}

tree_t *make_tree( program_t *prg, tree_t **args, long nargs )
{
	value_t id_int = (value_t)args[0];

	long id = (long)id_int;
	struct lang_el_info *lel_info = prg->rtd->lel_info;

	tree_t *tree = tree_allocate( prg );
	tree->id = id;
	tree->refs = 1;

	long object_length = lel_info[id].object_length;
	kid_t *attrs = alloc_attrs( prg, object_length );

	kid_t *last = 0, *child = 0;
	for ( id = 1; id < nargs; id++ ) {
		kid_t *kid = kid_allocate( prg );
		kid->tree = args[id];
		colm_tree_upref( kid->tree );

		if ( last == 0 )
			child = kid;
		else
			last->next = kid;

		last = kid;
	}

	tree->child = kid_list_concat( attrs, child );

	return tree;
}

int test_false( program_t *prg, tree_t *tree )
{
	int flse = ( 
		tree == 0 ||
		tree == prg->false_val 
	);
	return flse;
}

kid_t *copy_ignore_list( program_t *prg, kid_t *ignore_header )
{
	kid_t *new_header = kid_allocate( prg );
	kid_t *last = 0, *ic = (kid_t*)ignore_header->tree;
	while ( ic != 0 ) {
		kid_t *new_ic = kid_allocate( prg );

		new_ic->tree = ic->tree;
		new_ic->tree->refs += 1;

		/* List pointers. */
		if ( last == 0 )
			new_header->tree = (tree_t*)new_ic;
		else
			last->next = new_ic;

		ic = ic->next;
		last = new_ic;
	}
	return new_header;
}

kid_t *copy_kid_list( program_t *prg, kid_t *kid_list )
{
	kid_t *new_list = 0, *last = 0, *ic = kid_list;

	while ( ic != 0 ) {
		kid_t *new_ic = kid_allocate( prg );

		new_ic->tree = ic->tree;
		colm_tree_upref( new_ic->tree );

		/* List pointers. */
		if ( last == 0 )
			new_list = new_ic;
		else
			last->next = new_ic;

		ic = ic->next;
		last = new_ic;
	}
	return new_list;
}

/* New tree has zero ref. */
tree_t *copy_real_tree( program_t *prg, tree_t *tree, kid_t *old_next_down, kid_t **new_next_down )
{
	/* Need to keep a lookout for next down. If 
	 * copying it, return the copy. */
	tree_t *new_tree = tree_allocate( prg );

	new_tree->id = tree->id;
	new_tree->tokdata = string_copy( prg, tree->tokdata );
	new_tree->prod_num = tree->prod_num;

	/* Copy the child list. Start with ignores, then the list. */
	kid_t *child = tree->child, *last = 0;

	/* Left ignores. */
	if ( tree->flags & AF_LEFT_IGNORE ) {
		new_tree->flags |= AF_LEFT_IGNORE;
//		kid_t *newHeader = copyIgnoreList( prg, child );
//
//		/* Always the head. */
//		newTree->child = newHeader;
//
//		child = child->next;
//		last = newHeader;
	}

	/* Right ignores. */
	if ( tree->flags & AF_RIGHT_IGNORE ) {
		new_tree->flags |= AF_RIGHT_IGNORE;
//		kid_t *newHeader = copyIgnoreList( prg, child );
//		if ( last == 0 )
//			newTree->child = newHeader;
//		else
//			last->next = newHeader;
//		child = child->next;
//		last = newHeader;
	}

	/* Attributes and children. */
	while ( child != 0 ) {
		kid_t *new_kid = kid_allocate( prg );

		/* Watch out for next down. */
		if ( child == old_next_down )
			*new_next_down = new_kid;

		new_kid->tree = child->tree;
		new_kid->next = 0;

		/* May be an attribute. */
		if ( new_kid->tree != 0 )
			new_kid->tree->refs += 1;

		/* Store the first child. */
		if ( last == 0 )
			new_tree->child = new_kid;
		else
			last->next = new_kid;

		child = child->next;
		last = new_kid;
	}
	
	return new_tree;
}


tree_t *colm_copy_tree( program_t *prg, tree_t *tree, kid_t *old_next_down, kid_t **new_next_down )
{
//	struct lang_el_info *lelInfo = prg->rtd->lelInfo;
//	long genericId = lelInfo[tree->id].genericId;
//	if ( genericId > 0 )
//		assert(false);
	if ( tree->id == LEL_ID_PTR )
		assert(false);
//	else if ( tree->id == LEL_ID_BOOL )
//		assert(false);
//	else if ( tree->id == LEL_ID_INT )
//		assert(false);
	else if ( tree->id == LEL_ID_STR )
		assert(false);
//	else if ( tree->id == LEL_ID_STREAM )
//		assert(false);
	else {
		tree = copy_real_tree( prg, tree, old_next_down, new_next_down );
	}

	assert( tree->refs == 0 );
	return tree;
}

tree_t *split_tree( program_t *prg, tree_t *tree )
{
	if ( tree != 0 ) {
		assert( tree->refs >= 1 );

		if ( tree->refs > 1 ) {
			kid_t *old_next_down = 0, *new_next_down = 0;
			tree_t *new_tree = colm_copy_tree( prg, tree, old_next_down, &new_next_down );
			colm_tree_upref( new_tree );

			/* Downref the original. Don't need to consider freeing because
			 * refs were > 1. */
			tree->refs -= 1;

			tree = new_tree;
		}

		assert( tree->refs == 1 );
	}
	return tree;
}

/* We can't make recursive calls here since the tree we are freeing may be
 * very large. Need the VM stack. */
void tree_free_rec( program_t *prg, tree_t **sp, tree_t *tree )
{
	tree_t **top = vm_ptop();
//	struct lang_el_info *lelInfo;
//	long genericId;

free_tree:
//	lelInfo = prg->rtd->lelInfo;
//	genericId = lelInfo[tree->id].genericId;
//	assert( genericId == 0 );

	switch ( tree->id ) {
//	case LEL_ID_BOOL:
//	case LEL_ID_INT:
	case LEL_ID_PTR:
		tree_free( prg, tree );
		break;
	case LEL_ID_STR: {
		str_t *str = (str_t*) tree;
		string_free( prg, str->value );
		tree_free( prg, tree );
		break;
	}
	default: { 
		if ( tree->id != LEL_ID_IGNORE )
			string_free( prg, tree->tokdata );

		/* Attributes and grammar-based children. */
		kid_t *child = tree->child;
		while ( child != 0 ) {
			kid_t *next = child->next;
			vm_push_tree( child->tree );
			kid_free( prg, child );
			child = next;
		}

		tree_free( prg, tree );
		break;
	}}

	/* Any trees to downref? */
	while ( sp != top ) {
		tree = vm_pop_tree();
		if ( tree != 0 ) {
			assert( tree->refs > 0 );
			tree->refs -= 1;
			if ( tree->refs == 0 )
				goto free_tree;
		}
	}
}

void colm_tree_upref( tree_t *tree )
{
	if ( tree != 0 )
		tree->refs += 1;
}

void colm_tree_downref( program_t *prg, tree_t **sp, tree_t *tree )
{
	if ( tree != 0 ) {
		assert( tree->refs > 0 );
		tree->refs -= 1;
		if ( tree->refs == 0 )
			tree_free_rec( prg, sp, tree );
	}
}

/* We can't make recursive calls here since the tree we are freeing may be
 * very large. Need the VM stack. */
void object_free_rec( program_t *prg, tree_t **sp, tree_t *tree )
{
	tree_t **top = vm_ptop();
//	struct lang_el_info *lelInfo;
//	long genericId;

free_tree:
//	lelInfo = prg->rtd->lelInfo;

	switch ( tree->id ) {
	case LEL_ID_STR: {
		str_t *str = (str_t*) tree;
		string_free( prg, str->value );
		tree_free( prg, tree );
		break;
	}
//	case LEL_ID_BOOL:
//	case LEL_ID_INT: {
//		tree_free( prg, tree );
//		break;
//	}
	case LEL_ID_PTR: {
		tree_free( prg, tree );
		break;
	}
//	case LEL_ID_STREAM: {
//	}
	default: { 
		if ( tree->id != LEL_ID_IGNORE )
			string_free( prg, tree->tokdata );

		/* Attributes and grammar-based children. */
		kid_t *child = tree->child;
		while ( child != 0 ) {
			kid_t *next = child->next;
			vm_push_tree( child->tree );
			kid_free( prg, child );
			child = next;
		}

		tree_free( prg, tree );
		break;
	}}

	/* Any trees to downref? */
	while ( sp != top ) {
		tree = vm_pop_tree();
		if ( tree != 0 ) {
			assert( tree->refs > 0 );
			tree->refs -= 1;
			if ( tree->refs == 0 )
				goto free_tree;
		}
	}
}

void object_downref( program_t *prg, tree_t **sp, tree_t *tree )
{
	if ( tree != 0 ) {
		assert( tree->refs > 0 );
		tree->refs -= 1;
		if ( tree->refs == 0 )
			object_free_rec( prg, sp, tree );
	}
}

/* Find the first child of a tree. */
kid_t *tree_child( program_t *prg, const tree_t *tree )
{
	struct lang_el_info *lel_info = prg->rtd->lel_info;
	kid_t *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	/* Skip over attributes. */
	long object_length = lel_info[tree->id].object_length;
	long a;
	for ( a = 0; a < object_length; a++ )
		kid = kid->next;

	return kid;
}

/* Detach at the first real child of a tree. */
kid_t *tree_extract_child( program_t *prg, tree_t *tree )
{
	struct lang_el_info *lel_info = prg->rtd->lel_info;
	kid_t *kid = tree->child, *last = 0;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	/* Skip over attributes. */
	long a, object_length = lel_info[tree->id].object_length;
	for ( a = 0; a < object_length; a++ ) {
		last = kid;
		kid = kid->next;
	}

	if ( last == 0 )
		tree->child = 0;
	else
		last->next = 0;

	return kid;
}


/* Find the first child of a tree. */
kid_t *tree_attr( program_t *prg, const tree_t *tree )
{
	kid_t *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	return kid;
}

tree_t *tree_left_ignore( program_t *prg, tree_t *tree )
{
	if ( tree->flags & AF_LEFT_IGNORE )
		return tree->child->tree;
	return 0;
}

tree_t *tree_right_ignore( program_t *prg, tree_t *tree )
{
	if ( tree->flags & AF_RIGHT_IGNORE ) {
		if ( tree->flags & AF_LEFT_IGNORE )
			return tree->child->next->tree;
		else
			return tree->child->tree;
	}
	return 0;
}

kid_t *tree_left_ignore_kid( program_t *prg, tree_t *tree )
{
	if ( tree->flags & AF_LEFT_IGNORE )
		return tree->child;
	return 0;
}

kid_t *tree_right_ignore_kid( program_t *prg, tree_t *tree )
{
	if ( tree->flags & AF_RIGHT_IGNORE ) {
		if ( tree->flags & AF_LEFT_IGNORE )
			return tree->child->next;
		else
			return tree->child;
	}
	return 0;
}

void ref_set_value( program_t *prg, tree_t **sp, ref_t *ref, tree_t *v )
{
	colm_tree_downref( prg, sp, ref->kid->tree );
	ref->kid->tree = v;
}

tree_t *get_rhs_el( program_t *prg, tree_t *lhs, long position )
{
	kid_t *pos = tree_child( prg, lhs );
	while ( position > 0 ) {
		pos = pos->next;
		position -= 1;
	}
	return pos->tree;
}

kid_t *get_rhs_el_kid( program_t *prg, tree_t *lhs, long position )
{
	kid_t *pos = tree_child( prg, lhs );
	while ( position > 0 ) {
		pos = pos->next;
		position -= 1;
	}
	return pos;
}

parse_tree_t *get_rhs_parse_tree( program_t *prg, parse_tree_t *lhs, long position )
{
	parse_tree_t *pos = lhs->child;
	while ( position > 0 ) {
		pos = pos->next;
		position -= 1;
	}
	return pos;
}

tree_t *colm_get_rhs_val( program_t *prg, tree_t *tree, int *a )
{
	int i, len = a[0];
	for ( i = 0; i < len; i++ ) {
		int prod_num = a[1 + i * 2];
		int child_num = a[1 + i * 2 + 1];
		if ( tree->prod_num == prod_num )
			return get_rhs_el( prg, tree, child_num );
	}
	return 0;
}

void colm_tree_set_field( program_t *prg, tree_t *tree, long field, tree_t *value )
{
	assert( tree->refs == 1 );
	if ( value != 0 )
		assert( value->refs >= 1 );
	colm_tree_set_attr( tree, field, value );
}

tree_t *colm_tree_get_field( tree_t *tree, word_t field )
{
	return colm_get_attr( tree, field );
}

kid_t *get_field_kid( tree_t *tree, word_t field )
{
	return get_attr_kid( tree, field );
}

tree_t *get_field_split( program_t *prg, tree_t *tree, word_t field )
{
	tree_t *val = colm_get_attr( tree, field );
	tree_t *split = split_tree( prg, val );
	colm_tree_set_attr( tree, field, split );
	return split;
}

/* This must traverse in the same order that the bindId assignments are done
 * in. */
int match_pattern( tree_t **bindings, program_t *prg, long pat, kid_t *kid, int check_next )
{
	struct pat_cons_node *nodes = prg->rtd->pat_repl_nodes;

	/* match node, recurse on children. */
	if ( pat != -1 && kid != 0 ) {
		if ( nodes[pat].id == kid->tree->id ) {
			/* If the pattern node has data, then this means we need to match
			 * the data against the token data. */
			if ( nodes[pat].data != 0 ) {
				/* Check the length of token text. */
				if ( nodes[pat].length != string_length( kid->tree->tokdata ) )
					return false;

				/* Check the token text data. */
				if ( nodes[pat].length > 0 && memcmp( nodes[pat].data, 
						string_data( kid->tree->tokdata ), nodes[pat].length ) != 0 )
					return false;
			}

			/* No failure, all okay. */
			if ( nodes[pat].bind_id > 0 ) {
				bindings[nodes[pat].bind_id] = kid->tree;
			}

			/* If we didn't match a terminal duplicate of a nonterm then check
			 * down the children. */
			if ( !nodes[pat].stop ) {
				/* Check for failure down child branch. */
				int child_check = match_pattern( bindings, prg, 
						nodes[pat].child, tree_child( prg, kid->tree ), true );
				if ( ! child_check )
					return false;
			}

			/* If checking next, then look for failure there. */
			if ( check_next ) {
				int next_check = match_pattern( bindings, prg, 
						nodes[pat].next, kid->next, true );
				if ( ! next_check )
					return false;
			}

			return true;
		}
	}
	else if ( pat == -1 && kid == 0 ) {
		/* Both null is a match. */
		return 1;
	}

	return false;
}


long colm_cmp_tree( program_t *prg, const tree_t *tree1, const tree_t *tree2 )
{
	long cmpres = 0;
	if ( tree1 == 0 ) {
		if ( tree2 == 0 )
			return 0;
		else
			return -1;
	}
	else if ( tree2 == 0 )
		return 1;
	else if ( tree1->id < tree2->id )
		return -1;
	else if ( tree1->id > tree2->id )
		return 1;
	else if ( tree1->id == LEL_ID_PTR ) {
		if ( ((pointer_t*)tree1)->value < ((pointer_t*)tree2)->value )
			return -1;
		else if ( ((pointer_t*)tree1)->value > ((pointer_t*)tree2)->value )
			return 1;
	}
	else if ( tree1->id == LEL_ID_STR ) {
		cmpres = cmp_string( ((str_t*)tree1)->value, ((str_t*)tree2)->value );
		if ( cmpres != 0 )
			return cmpres;
	}
	else {
		if ( tree1->tokdata == 0 && tree2->tokdata != 0 )
			return -1;
		else if ( tree1->tokdata != 0 && tree2->tokdata == 0 )
			return 1;
		else if ( tree1->tokdata != 0 && tree2->tokdata != 0 ) {
			cmpres = cmp_string( tree1->tokdata, tree2->tokdata );
			if ( cmpres != 0 )
				return cmpres;
		}
	}

	kid_t *kid1 = tree_child( prg, tree1 );
	kid_t *kid2 = tree_child( prg, tree2 );

	while ( true ) {
		if ( kid1 == 0 && kid2 == 0 )
			return 0;
		else if ( kid1 == 0 && kid2 != 0 )
			return -1;
		else if ( kid1 != 0 && kid2 == 0 )
			return 1;
		else {
			cmpres = colm_cmp_tree( prg, kid1->tree, kid2->tree );
			if ( cmpres != 0 )
				return cmpres;
		}
		kid1 = kid1->next;
		kid2 = kid2->next;
	}
}


void split_ref( program_t *prg, tree_t ***psp, ref_t *from_ref )
{
	/* Go up the chain of kids, turing the pointers down. */
	ref_t *last = 0, *ref = from_ref, *next = 0;
	while ( ref->next != 0 ) {
		next = ref->next;
		ref->next = last;
		last = ref;
		ref = next;
	}
	ref->next = last;

	/* Now traverse the list, which goes down. */
	while ( ref != 0 ) {
		if ( ref->kid->tree->refs > 1 ) {
			ref_t *next_down = ref->next;
			while ( next_down != 0 && next_down->kid == ref->kid )
				next_down = next_down->next;

			kid_t *old_next_kid_down = next_down != 0 ? next_down->kid : 0;
			kid_t *new_next_kid_down = 0;

			tree_t *new_tree = colm_copy_tree( prg, ref->kid->tree, 
					old_next_kid_down, &new_next_kid_down );
			colm_tree_upref( new_tree );
			
			/* Downref the original. Don't need to consider freeing because
			 * refs were > 1. */
			ref->kid->tree->refs -= 1;

			while ( ref != 0 && ref != next_down ) {
				next = ref->next;
				ref->next = 0;

				ref->kid->tree = new_tree;
				ref = next;
			}

			/* Correct kid pointers down from ref. */
			while ( next_down != 0 && next_down->kid == old_next_kid_down ) {
				next_down->kid = new_next_kid_down;
				next_down = next_down->next;
			}
		}
		else {
			/* Reset the list as we go down. */
			next = ref->next;
			ref->next = 0;
			ref = next;
		}
	}
}

tree_t *set_list_mem( list_t *list, half_t field, tree_t *value )
{
	if ( value != 0 )
		assert( value->refs >= 1 );

	tree_t *existing = 0;
	switch ( field ) {
		case 0:
//			existing = list->head->value;
//			list->head->value = value;
			break;
		case 1:
//			existing = list->tail->value;
//			list->tail->value = value;
			break;
		default:
			assert( false );
			break;
	}
	return existing;
}

struct tree_pair map_remove( program_t *prg, map_t *map, tree_t *key )
{
	map_el_t *map_el = map_impl_find( prg, map, key );
	struct tree_pair result = { 0, 0 };
	if ( map_el != 0 ) {
		map_detach( prg, map, map_el );
		result.key = map_el->key;
		//mapElFree( prg, mapEl );
	}

	return result;
}

#if 0
tree_t *map_unstore( program_t *prg, map_t *map, tree_t *key, tree_t *existing )
{
	tree_t *stored = 0;
	if ( existing == 0 ) {
		map_el_t *map_el = map_detach_by_key( prg, map, key );
		// stored = mapEl->tree;
		map_el_free( prg, map_el );
	}
	else {
		map_el_t *map_el = map_impl_find( prg, map, key );
		// stored = mapEl->tree;
		//mapEl->tree = existing;
	}
	return stored;
}
#endif

tree_t *map_find( program_t *prg, map_t *map, tree_t *key )
{
//	map_el_t *mapEl = mapImplFind( prg, map, key );
//	return mapEl == 0 ? 0 : mapEl->tree;
	return 0;
}

long map_length( map_t *map )
{
	return map->tree_size;
}

void list_push_tail( program_t *prg, list_t *list, tree_t *val )
{
//	if ( val != 0 )
//		assert( val->refs >= 1 );
//	list_el_t *listEl = colm_list_el_new( prg );
//	listEl->value = val;
//	listAppend( list, listEl );
}

void list_push_head( program_t *prg, list_t *list, tree_t *val )
{
//	if ( val != 0 )
//		assert( val->refs >= 1 );
//	list_el_t *listEl = listElAllocate( prg );
//	listEl->value = val;
//	listPrepend( list, listEl );
}

tree_t *list_remove_end( program_t *prg, list_t *list )
{
//	tree_t *tree = list->tail->value;
//	listElFree( prg, listDetachLast( list ) );
//	return tree;
	return 0;
}

tree_t *list_remove_head( program_t *prg, list_t *list )
{
//	tree_t *tree = list->head;
//	listDetachFirst( list );
//	return tree;
	return 0;
}

tree_t *get_parser_mem( parser_t *parser, word_t field )
{
	tree_t *result = 0;
	switch ( field ) {
		case 0: {
			result = parser->result;
			break;
		}
		case 1: {
			struct pda_run *pda_run = parser->pda_run;
			result = pda_run->parse_error_text;
			break;
		}
		default: {
			assert( false );
			break;
		}
	}
	return result;
}

tree_t *get_list_mem_split( program_t *prg, list_t *list, word_t field )
{
	tree_t *sv = 0;
	switch ( field ) {
		case 0: 
//			sv = splitTree( prg, list->head->value );
//			list->head->value = sv; 
			break;
		case 1: 
//			sv = splitTree( prg, list->tail->value );
//			list->tail->value = sv; 
			break;
		default:
			assert( false );
			break;
	}
	return sv;
}


#if 0
int map_insert( program_t *prg, map_t *map, tree_t *key, tree_t *element )
{
	map_el_t *map_el = map_insert_key( prg, map, key, 0 );

	if ( map_el != 0 ) {
		//mapEl->tree = element;
		return true;
	}

	return false;
}
#endif

#if 0
void map_unremove( program_t *prg, map_t *map, tree_t *key, tree_t *element )
{
	map_el_t *map_el = map_insert_key( prg, map, key, 0 );
	assert( map_el != 0 );
	//mapEl->tree = element;
}
#endif

#if 0
tree_t *map_uninsert( program_t *prg, map_t *map, tree_t *key )
{
	map_el_t *el = map_detach_by_key( prg, map, key );
//	tree_t *val = el->tree;
	map_el_free( prg, el );
//	return val;
	return 0;
}
#endif

#if 0
tree_t *map_store( program_t *prg, map_t *map, tree_t *key, tree_t *element )
{
	tree_t *old_tree = 0;
	map_el_t *el_in_tree = 0;
	map_el_t *map_el = map_insert_key( prg, map, key, &el_in_tree );

//	if ( mapEl != 0 )
//		mapEl->tree = element;
//	else {
//		/* Element with key exists. Overwriting the value. */
//		oldTree = elInTree->tree;
//		elInTree->tree = element;
//	}

	return old_tree;
}
#endif

static tree_t *tree_search_kid( program_t *prg, kid_t *kid, long id )
{
	/* This node the one? */
	if ( kid->tree->id == id )
		return kid->tree;

	tree_t *res = 0;

	/* Search children. */
	kid_t *child = tree_child( prg, kid->tree );
	if ( child != 0 )
		res = tree_search_kid( prg, child, id );
	
	/* Search siblings. */
	if ( res == 0 && kid->next != 0 )
		res = tree_search_kid( prg, kid->next, id );

	return res;	
}

tree_t *tree_search( program_t *prg, tree_t *tree, long id )
{
	tree_t *res = 0;
	if ( tree->id == id )
		res = tree;
	else {
		kid_t *child = tree_child( prg, tree );
		if ( child != 0 )
			res = tree_search_kid( prg, child, id );
	}
	return res;
}

static location_t *loc_search_kid( program_t *prg, kid_t *kid )
{
	/* This node the one? */
	if ( kid->tree->tokdata != 0 && kid->tree->tokdata->location != 0 )
		return kid->tree->tokdata->location;

	location_t *res = 0;

	/* Search children. */
	kid_t *child = tree_child( prg, kid->tree );
	if ( child != 0 )
		res = loc_search_kid( prg, child );
	
	/* Search siblings. */
	if ( res == 0 && kid->next != 0 )
		res = loc_search_kid( prg, kid->next );

	return res;	
}

static location_t *loc_search( program_t *prg, tree_t *tree )
{
	location_t *res = 0;
	if ( tree->tokdata != 0 && tree->tokdata->location != 0 )
		return tree->tokdata->location;

	kid_t *child = tree_child( prg, tree );
	if ( child != 0 )
		res = loc_search_kid( prg, child );

	return res;
}

struct colm_location *colm_find_location( program_t *prg, tree_t *tree )
{
	return loc_search( prg, tree );
}

/*
 * tree_t Printing
 */

void xml_escape_data( struct colm_print_args *print_args, const char *data, long len )
{
	int i;
	for ( i = 0; i < len; i++ ) {
		if ( data[i] == '<' )
			print_args->out( print_args, "&lt;", 4 );
		else if ( data[i] == '>' )
			print_args->out( print_args, "&gt;", 4 );
		else if ( data[i] == '&' )
			print_args->out( print_args, "&amp;", 5 );
		else if ( (32 <= data[i] && data[i] <= 126) || 
				data[i] == '\t' || data[i] == '\n' || data[i] == '\r' )
		{
			print_args->out( print_args, &data[i], 1 );
		}
		else {
			char out[64];
			sprintf( out, "&#%u;", ((unsigned)data[i]) );
			print_args->out( print_args, out, strlen(out) );
		}
	}
}

void init_str_collect( StrCollect *collect )
{
	collect->data = (char*) malloc( BUFFER_INITIAL_SIZE );
	collect->allocated = BUFFER_INITIAL_SIZE;
	collect->length = 0;
}

void str_collect_destroy( StrCollect *collect )
{
	free( collect->data );
}

void str_collect_append( StrCollect *collect, const char *data, long len )
{
	long new_len = collect->length + len;
	if ( new_len > collect->allocated ) {
		collect->allocated = new_len * 2;
		collect->data = (char*) realloc( collect->data, collect->allocated );
	}
	memcpy( collect->data + collect->length, data, len );
	collect->length += len;
}
		
void str_collect_clear( StrCollect *collect )
{
	collect->length = 0;
}

#define INT_SZ 32

void print_str( struct colm_print_args *print_args, head_t *str )
{
	print_args->out( print_args, (char*)(str->data), str->length );
}

void append_collect( struct colm_print_args *args, const char *data, int length )
{
	str_collect_append( (StrCollect*) args->arg, data, length );
}

void append_file( struct colm_print_args *args, const char *data, int length )
{
	int level;
	struct stream_impl *impl = (struct stream_impl*) args->arg;

restart:
	if ( impl->indent ) {
		/* Consume mode. */
		while ( length > 0 && ( *data == ' ' || *data == '\t' ) ) {
			data += 1;
			length -= 1;
		}

		if ( length > 0 ) {
			/* Found some data, print the indentation and turn off indentation
			 * mode. */
			for ( level = 0; level < impl->level; level++ )
				fputc( '\t', impl->file );

			impl->indent = 0;

			goto restart;
		}
	}
	else {
		char *nl;
		if ( impl->level != COLM_INDENT_OFF &&
				(nl = memchr( data, '\n', length )) )
		{
			/* Print up to and including the newline. */
			int wl = nl - data + 1;
			fwrite( data, 1, wl, impl->file );

			/* Go into consume state. If we see more non-indentation chars we
			 * will generate the appropriate indentation level. */
			data += wl;
			length -= wl;
			impl->indent = 1;
			goto restart;
		}
		else {
			/* Indentation off, or no indent trigger (newline). */
			fwrite( data, 1, length, impl->file );
		}
	}
}


tree_t *tree_trim( struct colm_program *prg, tree_t **sp, tree_t *tree )
{
	debug( prg, REALM_PARSE, "attaching left ignore\n" );

	/* Make the ignore list for the left-ignore. */
	tree_t *left_ignore = tree_allocate( prg );
	left_ignore->id = LEL_ID_IGNORE;
	left_ignore->flags |= AF_SUPPRESS_RIGHT;

	tree = push_left_ignore( prg, tree, left_ignore );

	debug( prg, REALM_PARSE, "attaching ignore right\n" );

	/* Copy the ignore list first if we need to attach it as a right
	 * ignore. */
	tree_t *right_ignore = 0;
	right_ignore = tree_allocate( prg );
	right_ignore->id = LEL_ID_IGNORE;
	right_ignore->flags |= AF_SUPPRESS_LEFT;

	tree = push_right_ignore( prg, tree, right_ignore );

	return tree;
}

enum ReturnType
{
	Done = 1,
	CollectIgnoreLeft,
	CollectIgnoreRight,
	RecIgnoreList,
	ChildPrint
};

enum VisitType
{
	IgnoreWrapper,
	IgnoreData,
	Term,
	NonTerm,
};

#define TF_TERM_SEEN 0x1

void print_kid( program_t *prg, tree_t **sp, struct colm_print_args *print_args, kid_t *kid )
{
	enum ReturnType rt;
	kid_t *parent = 0;
	kid_t *leading_ignore = 0;
	enum VisitType visit_type;
	int flags = 0;

	/* Iterate the kids passed in. We are expecting a next, which will allow us
	 * to print the trailing ignore list. */
	while ( kid != 0 ) {
		vm_push_type( enum ReturnType, Done );
		goto rec_call;
		rec_return_top:
		kid = kid->next;
	}

	return;

rec_call:
	if ( kid->tree == 0 )
		goto skip_null;

	/* If not currently skipping ignore data, then print it. Ignore data can
	 * be associated with terminals and nonterminals. */
	if ( kid->tree->flags & AF_LEFT_IGNORE ) {
		vm_push_kid( parent );
		vm_push_kid( kid );
		parent = kid;
		kid = tree_left_ignore_kid( prg, kid->tree );
		vm_push_type( enum ReturnType, CollectIgnoreLeft );
		goto rec_call;
		rec_return_ign_left:
		kid = vm_pop_kid();
		parent = vm_pop_kid();
	}

	if ( kid->tree->id == LEL_ID_IGNORE )
		visit_type = IgnoreWrapper;
	else if ( parent != 0 && parent->tree->id == LEL_ID_IGNORE )
		visit_type = IgnoreData;
	else if ( kid->tree->id < prg->rtd->first_non_term_id )
		visit_type = Term;
	else
		visit_type = NonTerm;
	
	debug( prg, REALM_PRINT, "visit type: %d\n", visit_type );

	if ( visit_type == IgnoreData ) {
		debug( prg, REALM_PRINT, "putting %p on ignore list\n", kid->tree );
		kid_t *new_ignore = kid_allocate( prg );
		new_ignore->next = leading_ignore;
		leading_ignore = new_ignore;
		leading_ignore->tree = kid->tree;
		goto skip_node;
	}

	if ( visit_type == IgnoreWrapper ) {
		kid_t *new_ignore = kid_allocate( prg );
		new_ignore->next = leading_ignore;
		leading_ignore = new_ignore;
		leading_ignore->tree = kid->tree;
		/* Don't skip. */
	}

	/* print leading ignore? Triggered by terminals. */
	if ( visit_type == Term ) {
		/* Reverse the leading ignore list. */
		if ( leading_ignore != 0 ) {
			kid_t *ignore = 0, *last = 0;

			/* Reverse the list and take the opportunity to implement the
			 * suppress left. */
			while ( true ) {
				kid_t *next = leading_ignore->next;
				leading_ignore->next = last;

				if ( leading_ignore->tree->flags & AF_SUPPRESS_LEFT ) {
					/* We are moving left. Chop off the tail. */
					debug( prg, REALM_PRINT, "suppressing left\n" );
					free_kid_list( prg, next );
					break;
				}

				if ( next == 0 )
					break;

				last = leading_ignore;
				leading_ignore = next;
			}

			/* Print the leading ignore list. Also implement the suppress right
			 * in the process. */
			if ( print_args->comm && (!print_args->trim ||
					(flags & TF_TERM_SEEN && kid->tree->id > 0)) )
			{
				ignore = leading_ignore;
				while ( ignore != 0 ) {
					if ( ignore->tree->flags & AF_SUPPRESS_RIGHT )
						break;

					if ( ignore->tree->id != LEL_ID_IGNORE ) {
						vm_push_type( enum VisitType, visit_type );
						vm_push_kid( leading_ignore );
						vm_push_kid( ignore );
						vm_push_kid( parent );
						vm_push_kid( kid );

						leading_ignore = 0;
						kid = ignore;
						parent = 0;

						debug( prg, REALM_PRINT, "rec call on %p\n", kid->tree );
						vm_push_type( enum ReturnType, RecIgnoreList );
						goto rec_call;
						rec_return_il:

						kid = vm_pop_kid();
						parent = vm_pop_kid();
						ignore = vm_pop_kid();
						leading_ignore = vm_pop_kid();
						visit_type = vm_pop_type(enum VisitType);
					}

					ignore = ignore->next;
				}
			}

			/* Free the leading ignore list. */
			free_kid_list( prg, leading_ignore );
			leading_ignore = 0;
		}
	}

	if ( visit_type == Term || visit_type == NonTerm ) {
		/* Open the tree. */
		print_args->open_tree( prg, sp, print_args, parent, kid );
	}

	if ( visit_type == Term )
		flags |= TF_TERM_SEEN;

	if ( visit_type == Term || visit_type == IgnoreData ) {
		/* Print contents. */
		if ( kid->tree->id < prg->rtd->first_non_term_id ) {
			debug( prg, REALM_PRINT, "printing terminal %p\n", kid->tree );
			if ( kid->tree->id != 0 )
				print_args->print_term( prg, sp, print_args, kid );
		}
	}

	/* Print children. */
	kid_t *child = print_args->attr ? 
		tree_attr( prg, kid->tree ) : 
		tree_child( prg, kid->tree );

	if ( child != 0 ) {
		vm_push_type( enum VisitType, visit_type );
		vm_push_kid( parent );
		vm_push_kid( kid );
		parent = kid;
		kid = child;
		while ( kid != 0 ) {
			vm_push_type( enum ReturnType, ChildPrint );
			goto rec_call;
			rec_return:
			kid = kid->next;
		}
		kid = vm_pop_kid();
		parent = vm_pop_kid();
		visit_type = vm_pop_type(enum VisitType);
	}

	if ( visit_type == Term || visit_type == NonTerm ) {
		/* close the tree. */
		print_args->close_tree( prg, sp, print_args, parent, kid );
	}

skip_node:

	/* If not currently skipping ignore data, then print it. Ignore data can
	 * be associated with terminals and nonterminals. */
	if ( kid->tree->flags & AF_RIGHT_IGNORE ) {
		debug( prg, REALM_PRINT, "right ignore\n" );
		vm_push_kid( parent );
		vm_push_kid( kid );
		parent = kid;
		kid = tree_right_ignore_kid( prg, kid->tree );
		vm_push_type( enum ReturnType, CollectIgnoreRight );
		goto rec_call;
		rec_return_ign_right:
		kid = vm_pop_kid();
		parent = vm_pop_kid();
	}

/* For skiping over content on null. */
skip_null:

	rt = vm_pop_type(enum ReturnType);
	switch ( rt ) {
		case Done:
			debug( prg, REALM_PRINT, "return: done\n" );
			goto rec_return_top;
			break;
		case CollectIgnoreLeft:
			debug( prg, REALM_PRINT, "return: ignore left\n" );
			goto rec_return_ign_left;
		case CollectIgnoreRight:
			debug( prg, REALM_PRINT, "return: ignore right\n" );
			goto rec_return_ign_right;
		case RecIgnoreList:
			debug( prg, REALM_PRINT, "return: ignore list\n" );
			goto rec_return_il;
		case ChildPrint:
			debug( prg, REALM_PRINT, "return: child print\n" );
			goto rec_return;
	}
}

void colm_print_tree_args( program_t *prg, tree_t **sp,
		struct colm_print_args *print_args, tree_t *tree )
{
	if ( tree == 0 )
		print_args->out( print_args, "NIL", 3 );
	else {
		/* This term tree allows us to print trailing ignores. */
		tree_t term_tree;
		memset( &term_tree, 0, sizeof(term_tree) );

		kid_t kid, term;
		term.tree = &term_tree;
		term.next = 0;
		term.flags = 0;

		kid.tree = tree;
		kid.next = &term;
		kid.flags = 0;

		print_kid( prg, sp, print_args, &kid );
	}
}

void colm_print_term_tree( program_t *prg, tree_t **sp,
		struct colm_print_args *print_args, kid_t *kid )
{
	debug( prg, REALM_PRINT, "printing term %p\n", kid->tree );

	if ( kid->tree->id == LEL_ID_PTR ) {
		char buf[INT_SZ];
		print_args->out( print_args, "#<", 2 );
		sprintf( buf, "%lx", ((pointer_t*)kid->tree)->value );
		print_args->out( print_args, buf, strlen(buf) );
		print_args->out( print_args, ">", 1 );
	}
	else if ( kid->tree->id == LEL_ID_STR ) {
		print_str( print_args, ((str_t*)kid->tree)->value );
	}
//	else if ( kid->tree->id == LEL_ID_STREAM ) {
//		char buf[INT_SZ];
//		printArgs->out( printArgs, "#", 1 );
//		sprintf( buf, "%p", (void*) ((stream_t*)kid->tree)->in->file );
//		printArgs->out( printArgs, buf, strlen(buf) );
//	}
	else if ( kid->tree->tokdata != 0 && 
			string_length( kid->tree->tokdata ) > 0 )
	{
		print_args->out( print_args, string_data( kid->tree->tokdata ), 
				string_length( kid->tree->tokdata ) );
	}

	struct lang_el_info *lel_info = prg->rtd->lel_info;
	struct stream_impl *impl = (struct stream_impl*) print_args->arg;

	if ( strcmp( lel_info[kid->tree->id].name, "_IN_" ) == 0 ) {
		if ( impl->level == COLM_INDENT_OFF ) {
			impl->level = 1;
			impl->indent = 1;
		}
		else {
			impl->level += 1;
		}
	}

	if ( strcmp( lel_info[kid->tree->id].name, "_EX_" ) == 0 )
		impl->level -= 1;
}


void colm_print_null( program_t *prg, tree_t **sp,
		struct colm_print_args *args, kid_t *parent, kid_t *kid )
{
}

void open_tree_xml( program_t *prg, tree_t **sp, struct colm_print_args *args,
		kid_t *parent, kid_t *kid )
{
	/* Skip the terminal that is for forcing trailing ignores out. */
	if ( kid->tree->id == 0 )
		return;

	struct lang_el_info *lel_info = prg->rtd->lel_info;

	/* List flattening: skip the repeats and lists that are a continuation of
	 * the list. */
	if ( parent != 0 && parent->tree->id == kid->tree->id && kid->next == 0 &&
			( lel_info[parent->tree->id].repeat || lel_info[parent->tree->id].list ) )
	{
		return;
	}

	const char *name = lel_info[kid->tree->id].xml_tag;
	args->out( args, "<", 1 );
	args->out( args, name, strlen( name ) );
	args->out( args, ">", 1 );
}

void print_term_xml( program_t *prg, tree_t **sp,
	struct colm_print_args *print_args, kid_t *kid )
{
	//kid_t *child;

	/*child = */ tree_child( prg, kid->tree );
	if ( kid->tree->id == LEL_ID_PTR ) {
		char ptr[INT_SZ];
		sprintf( ptr, "%lx", ((pointer_t*)kid->tree)->value );
		print_args->out( print_args, ptr, strlen(ptr) );
	}
	else if ( kid->tree->id == LEL_ID_STR ) {
		head_t *head = (head_t*) ((str_t*)kid->tree)->value;

		xml_escape_data( print_args, (char*)(head->data), head->length );
	}
	else if ( 0 < kid->tree->id && kid->tree->id < prg->rtd->first_non_term_id &&
			kid->tree->id != LEL_ID_IGNORE &&
			kid->tree->tokdata != 0 && 
			string_length( kid->tree->tokdata ) > 0 )
	{
		xml_escape_data( print_args, string_data( kid->tree->tokdata ), 
				string_length( kid->tree->tokdata ) );
	}
}


void close_tree_xml( program_t *prg, tree_t **sp,
		struct colm_print_args *args, kid_t *parent, kid_t *kid )
{
	/* Skip the terminal that is for forcing trailing ignores out. */
	if ( kid->tree->id == 0 )
		return;

	struct lang_el_info *lel_info = prg->rtd->lel_info;

	/* List flattening: skip the repeats and lists that are a continuation of
	 * the list. */
	if ( parent != 0 && parent->tree->id == kid->tree->id && kid->next == 0 &&
			( lel_info[parent->tree->id].repeat || lel_info[parent->tree->id].list ) )
	{
		return;
	}

	const char *name = lel_info[kid->tree->id].xml_tag;
	args->out( args, "</", 2 );
	args->out( args, name, strlen( name ) );
	args->out( args, ">", 1 );
}

void print_tree_collect( program_t *prg, tree_t **sp,
		StrCollect *collect, tree_t *tree, int trim )
{
	struct colm_print_args print_args = {
			collect, true, false, trim, &append_collect, 
			&colm_print_null, &colm_print_term_tree, &colm_print_null
	};

	colm_print_tree_args( prg, sp, &print_args, tree );
}

void print_tree_collect_a( program_t *prg, tree_t **sp,
		StrCollect *collect, tree_t *tree, int trim )
{
	struct colm_print_args print_args = {
			collect, true, true, trim, &append_collect, 
			&colm_print_null, &colm_print_term_tree, &colm_print_null
	};

	colm_print_tree_args( prg, sp, &print_args, tree );
}

void print_tree_file( program_t *prg, tree_t **sp, struct stream_impl *impl,
		tree_t *tree, int trim )
{
	struct colm_print_args print_args = {
			impl, true, false, trim, &append_file, 
			&colm_print_null, &colm_print_term_tree, &colm_print_null
	};

	colm_print_tree_args( prg, sp, &print_args, tree );
}

void print_xml_stdout( program_t *prg, tree_t **sp,
		struct stream_impl *impl, tree_t *tree,
		int comm_attr, int trim )
{
	struct colm_print_args print_args = {
			impl, comm_attr, comm_attr, trim, &append_file, 
			&open_tree_xml, &print_term_xml, &close_tree_xml };
	colm_print_tree_args( prg, sp, &print_args, tree );
}
