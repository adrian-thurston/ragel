/*
 *  Copyright 2007-2014 Adrian Thurston <thurston@complang.org>
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

#include <colm/tree.h>
#include <colm/bytecode.h>
#include <colm/program.h>
#include <assert.h>
#include "internal.h"

#define true 1
#define false 0

void colm_init_list_iter( generic_iter_t *list_iter, tree_t **stack_root,
		long arg_size, long root_size, const ref_t *root_ref, int generic_id )
{
	list_iter->type = IT_Tree;
	list_iter->root_ref = *root_ref;
	list_iter->stack_root = stack_root;
	list_iter->yield_size = 0;
	list_iter->root_size = root_size;
	list_iter->ref.kid = 0;
	list_iter->ref.next = 0;
	list_iter->arg_size = arg_size;
	list_iter->generic_id = generic_id;
}

void colm_list_iter_destroy( program_t *prg, tree_t ***psp, generic_iter_t *iter )
{
	if ( (int)iter->type != 0 ) {
		int i;
		tree_t **sp = *psp;
		long cur_stack_size = vm_ssize() - iter->root_size;
		assert( iter->yield_size == cur_stack_size );
		vm_popn( iter->yield_size );
		for ( i = 0; i < iter->arg_size; i++ )
			colm_tree_downref( prg, sp, vm_pop_tree() );
		iter->type = 0;
		*psp = sp;
	}
}

tree_t *colm_list_iter_advance( program_t *prg, tree_t ***psp, generic_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yield_size == (vm_ssize() - iter->root_size) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		list_t *list = *((list_t**)iter->root_ref.kid);
		iter->ref.kid = (kid_t*)list->head;
		iter->ref.next = 0;

		//= iter->rootRef;
		//iter
		//iterFind( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		//iterFind( prg, psp, iter, false );

		list_el_t *list_el = (list_el_t*)iter->ref.kid;
		list_el = list_el->list_next;
		iter->ref.kid = (kid_t*)list_el;
		iter->ref.next = 0;
	}

	sp = *psp;
	iter->yield_size = vm_ssize() - iter->root_size;

	return (iter->ref.kid ? prg->true_val : prg->false_val );
}

tree_t *colm_rev_list_iter_advance( program_t *prg, tree_t ***psp, generic_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yield_size == (vm_ssize() - iter->root_size) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		list_t *list = *((list_t**)iter->root_ref.kid);
		iter->ref.kid = (kid_t*)list->tail;
		iter->ref.next = 0;

		//= iter->rootRef;
		//iter
		//iterFind( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		//iterFind( prg, psp, iter, false );

		list_el_t *list_el = (list_el_t*)iter->ref.kid;
		list_el = list_el->list_prev;
		iter->ref.kid = (kid_t*)list_el;
		iter->ref.next = 0;
	}

	sp = *psp;
	iter->yield_size = vm_ssize() - iter->root_size;

	return (iter->ref.kid ? prg->true_val : prg->false_val );
}

tree_t *colm_map_iter_advance( program_t *prg, tree_t ***psp, generic_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yield_size == (vm_ssize() - iter->root_size) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		map_t *map = *((map_t**)iter->root_ref.kid);
		iter->ref.kid = (kid_t*)map->head;
		iter->ref.next = 0;

		//= iter->rootRef;
		//iter
		//iterFind( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		//iterFind( prg, psp, iter, false );

		map_el_t *map_el = (map_el_t*)iter->ref.kid;
		map_el = map_el->next;
		iter->ref.kid = (kid_t*)map_el;
		iter->ref.next = 0;
	}

	sp = *psp;
	iter->yield_size = vm_ssize() - iter->root_size;

	return (iter->ref.kid ? prg->true_val : prg->false_val );
}

tree_t *colm_list_iter_deref_cur( program_t *prg, generic_iter_t *iter )
{
	struct generic_info *gi = &prg->rtd->generic_info[iter->generic_id];
	list_el_t *el = (list_el_t*)iter->ref.kid;
	struct colm_struct *s = el != 0 ?
			colm_struct_container( el, gi->el_offset ) : 0;
	return (tree_t*)s;
}

value_t colm_viter_deref_cur( program_t *prg, generic_iter_t *iter )
{
	struct generic_info *gi = &prg->rtd->generic_info[iter->generic_id];
	list_el_t *el = (list_el_t*)iter->ref.kid;
	struct colm_struct *s = el != 0 ?
			colm_struct_container( el, gi->el_offset ) : 0;

	value_t value = colm_struct_get_field( s, value_t, 0 );
	if ( gi->value_type == TYPE_TREE )
		colm_tree_upref( (tree_t*)value );

	return value;
}

void colm_init_tree_iter( tree_iter_t *tree_iter, tree_t **stack_root,
		long arg_size, long root_size,
		const ref_t *root_ref, int search_id )
{
	tree_iter->type = IT_Tree;
	tree_iter->root_ref = *root_ref;
	tree_iter->search_id = search_id;
	tree_iter->stack_root = stack_root;
	tree_iter->yield_size = 0;
	tree_iter->root_size = root_size;
	tree_iter->ref.kid = 0;
	tree_iter->ref.next = 0;
	tree_iter->arg_size = arg_size;
}

void colm_init_rev_tree_iter( rev_tree_iter_t *rev_triter, tree_t **stack_root,
		long arg_size, long root_size,
		const ref_t *root_ref, int search_id, int children )
{
	rev_triter->type = IT_RevTree;
	rev_triter->root_ref = *root_ref;
	rev_triter->search_id = search_id;
	rev_triter->stack_root = stack_root;
	rev_triter->yield_size = children;
	rev_triter->root_size = root_size;
	rev_triter->kid_at_yield = 0;
	rev_triter->children = children;
	rev_triter->ref.kid = 0;
	rev_triter->ref.next = 0;
	rev_triter->arg_size = arg_size;
}

void init_user_iter( user_iter_t *user_iter, tree_t **stack_root, long root_size,
		long arg_size, long search_id )
{
	user_iter->type = IT_User;
	user_iter->stack_root = stack_root;
	user_iter->arg_size = arg_size;
	user_iter->yield_size = 0;
	user_iter->root_size = root_size;
	user_iter->resume = 0;
	user_iter->frame = 0;
	user_iter->search_id = search_id;

	user_iter->ref.kid = 0;
	user_iter->ref.next = 0;
}


user_iter_t *colm_uiter_create( program_t *prg, tree_t ***psp, struct function_info *fi, long search_id )
{
	tree_t **sp = *psp;

	vm_pushn( sizeof(user_iter_t) / sizeof(word_t) );
	void *mem = vm_ptop();
	user_iter_t *uiter = mem;

	tree_t **stack_root = vm_ptop();
	long root_size = vm_ssize();

	init_user_iter( uiter, stack_root, root_size, fi->arg_size, search_id );

	*psp = sp;
	return uiter;
}

void uiter_init( program_t *prg, tree_t **sp, user_iter_t *uiter, 
		struct function_info *fi, int revert_on )
{
	/* Set up the first yeild so when we resume it starts at the beginning. */
	uiter->ref.kid = 0;
	uiter->yield_size = vm_ssize() - uiter->root_size;
	//	uiter->frame = &uiter->stackRoot[-IFR_AA];

	if ( revert_on )
		uiter->resume = prg->rtd->frame_info[fi->frame_id].codeWV;
	else
		uiter->resume = prg->rtd->frame_info[fi->frame_id].codeWC;
}


void colm_tree_iter_destroy( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	if ( (int)iter->type != 0 ) {
		int i;
		tree_t **sp = *psp;
		long cur_stack_size = vm_ssize() - iter->root_size;
		assert( iter->yield_size == cur_stack_size );
		vm_popn( iter->yield_size );
		for ( i = 0; i < iter->arg_size; i++ )
			colm_tree_downref( prg, sp, vm_pop_tree() );
		iter->type = 0;
		*psp = sp;
	}
}

void colm_rev_tree_iter_destroy( struct colm_program *prg, tree_t ***psp, rev_tree_iter_t *riter )
{
	if ( (int)riter->type != 0 ) {
		int i;
		tree_t **sp = *psp;
		long cur_stack_size = vm_ssize() - riter->root_size;
		assert( riter->yield_size == cur_stack_size );
		vm_popn( riter->yield_size );
		for ( i = 0; i < riter->arg_size; i++ )
			colm_tree_downref( prg, sp, vm_pop_tree() );
		riter->type = 0;
		*psp = sp;
	}
}

void colm_uiter_destroy( program_t *prg, tree_t ***psp, user_iter_t *uiter )
{
	if ( uiter != 0 && (int)uiter->type != 0 ) {
		tree_t **sp = *psp;

		/* We should always be coming from a yield. The current stack size will be
		 * nonzero and the stack size in the iterator will be correct. */
		long cur_stack_size = vm_ssize() - uiter->root_size;
		assert( uiter->yield_size == cur_stack_size );

		vm_popn( uiter->yield_size );
		vm_popn( sizeof(user_iter_t) / sizeof(word_t) );

		uiter->type = 0;

		*psp = sp;
	}
}

void colm_uiter_unwind( program_t *prg, tree_t ***psp, user_iter_t *uiter )
{
	if ( uiter != 0 && (int)uiter->type != 0 ) {
		tree_t **sp = *psp;

		/* We should always be coming from a yield. The current stack size will be
		 * nonzero and the stack size in the iterator will be correct. */
		long cur_stack_size = vm_ssize() - uiter->root_size;
		assert( uiter->yield_size == cur_stack_size );

		long arg_size = uiter->arg_size;

		vm_popn( uiter->yield_size );
		vm_popn( sizeof(user_iter_t) / sizeof(word_t) );

		/* The IN_PREP_ARGS stack data. */
		vm_popn( arg_size );
		vm_pop_value();

		uiter->type = 0;

		*psp = sp;
	}
}

tree_t *tree_iter_deref_cur( tree_iter_t *iter )
{
	return iter->ref.kid == 0 ? 0 : iter->ref.kid->tree;
}

void set_triter_cur( program_t *prg, tree_iter_t *iter, tree_t *tree )
{
	iter->ref.kid->tree = tree;
}

void set_uiter_cur( program_t *prg, user_iter_t *uiter, tree_t *tree )
{
	uiter->ref.kid->tree = tree;
}

void split_iter_cur( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	if ( iter->ref.kid == 0 )
		return;
	
	split_ref( prg, psp, &iter->ref );
}

void iter_find( program_t *prg, tree_t ***psp, tree_iter_t *iter, int try_first )
{
	int any_tree = iter->search_id == prg->rtd->any_id;
	tree_t **top = iter->stack_root;
	kid_t *child;
	tree_t **sp = *psp;

rec_call:
	if ( try_first && ( iter->ref.kid->tree->id == iter->search_id || any_tree ) ) {
		*psp = sp;
		return;
	}
	else {
		child = tree_child( prg, iter->ref.kid->tree );
		if ( child != 0 ) {
			vm_contiguous( 2 );
			vm_push_ref( iter->ref.next );
			vm_push_kid( iter->ref.kid );
			iter->ref.kid = child;
			iter->ref.next = (ref_t*)vm_ptop();
			while ( iter->ref.kid != 0 ) {
				try_first = true;
				goto rec_call;
				rec_return:
				iter->ref.kid = iter->ref.kid->next;
			}
			iter->ref.kid = vm_pop_kid();
			iter->ref.next = vm_pop_ref();
		}
	}

	if ( top != vm_ptop() )
		goto rec_return;
	
	iter->ref.kid = 0;
	*psp = sp;
}

tree_t *tree_iter_advance( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yield_size == (vm_ssize() - iter->root_size) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		iter->ref = iter->root_ref;
		iter_find( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iter_find( prg, psp, iter, false );
	}

	sp = *psp;
	iter->yield_size = vm_ssize() - iter->root_size;

	return (iter->ref.kid ? prg->true_val : prg->false_val );
}

tree_t *tree_iter_next_child( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yield_size == (vm_ssize() - iter->root_size) );
	kid_t *kid = 0;

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the first child. */
		kid_t *child = tree_child( prg, iter->root_ref.kid->tree );

		if ( child == 0 )
			iter->ref.next = 0;
		else {
			/* Make a reference to the root. */
			vm_contiguous( 2 );
			vm_push_ref( iter->root_ref.next );
			vm_push_kid( iter->root_ref.kid );
			iter->ref.next = (ref_t*)vm_ptop();

			kid = child;
		}
	}
	else {
		/* Start at next. */
		kid = iter->ref.kid->next;
	}

	if ( iter->search_id != prg->rtd->any_id ) {
		/* Have a previous item, go to the next sibling. */
		while ( kid != 0 && kid->tree->id != iter->search_id )
			kid = kid->next;
	}

	iter->ref.kid = kid;
	iter->yield_size = vm_ssize() - iter->root_size;
	*psp = sp;
	return ( iter->ref.kid ? prg->true_val : prg->false_val );
}

tree_t *tree_rev_iter_prev_child( program_t *prg, tree_t ***psp, rev_tree_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yield_size == ( vm_ssize() - iter->root_size ) );

	if ( iter->kid_at_yield != iter->ref.kid ) {
		/* Need to reload the kids. */
		vm_popn( iter->children );

		int c;
		kid_t *kid = tree_child( prg, iter->root_ref.kid->tree );
		for ( c = 0; c < iter->children; c++ ) {
			vm_push_kid( kid );
			kid = kid->next;
		}
	}

	if ( iter->ref.kid != 0 ) {
		vm_pop_ignore();
		iter->children -= 1;
	}

	if ( iter->search_id != prg->rtd->any_id ) {
		/* Have a previous item, go to the next sibling. */
		while ( iter->children > 0 && ((kid_t*)(vm_top()))->tree->id != iter->search_id ) {
			iter->children -= 1;
			vm_pop_ignore();
		}
	}

	if ( iter->children == 0 ) {
		iter->ref.next = 0;
		iter->ref.kid = 0;
	}
	else {
		iter->ref.next = &iter->root_ref;
		iter->ref.kid = (kid_t*)vm_top();
	}

	/* We will use this to detect a split above the iterated tree. */
	iter->kid_at_yield = iter->ref.kid;

	iter->yield_size = vm_ssize() - iter->root_size;

	*psp = sp;

	return (iter->ref.kid ? prg->true_val : prg->false_val );
}

void iter_find_repeat( program_t *prg, tree_t ***psp, tree_iter_t *iter, int try_first )
{
	tree_t **sp = *psp;
	int any_tree = iter->search_id == prg->rtd->any_id;
	tree_t **top = iter->stack_root;
	kid_t *child;

rec_call:
	if ( try_first && ( iter->ref.kid->tree->id == iter->search_id || any_tree ) ) {
		*psp = sp;
		return;
	}
	else {
		/* The repeat iterator is just like the normal top-down-left-right,
		 * execept it only goes into the children of a node if the node is the
		 * root of the iteration, or if does not have any neighbours to the
		 * right. */
		if ( top == vm_ptop() || iter->ref.kid->next == 0  ) {
			child = tree_child( prg, iter->ref.kid->tree );
			if ( child != 0 ) {
				vm_contiguous( 2 );
				vm_push_ref( iter->ref.next );
				vm_push_kid( iter->ref.kid );
				iter->ref.kid = child;
				iter->ref.next = (ref_t*)vm_ptop();
				while ( iter->ref.kid != 0 ) {
					try_first = true;
					goto rec_call;
					rec_return:
					iter->ref.kid = iter->ref.kid->next;
				}
				iter->ref.kid = vm_pop_kid();
				iter->ref.next = vm_pop_ref();
			}
		}
	}

	if ( top != vm_ptop() )
		goto rec_return;
	
	iter->ref.kid = 0;
	*psp = sp;
}

tree_t *tree_iter_next_repeat( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yield_size == ( vm_ssize() - iter->root_size ) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		iter->ref = iter->root_ref;
		iter_find_repeat( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iter_find_repeat( prg, psp, iter, false );
	}

	sp = *psp;
	iter->yield_size = vm_ssize() - iter->root_size;

	return (iter->ref.kid ? prg->true_val : prg->false_val );
}

void iter_find_rev_repeat( program_t *prg, tree_t ***psp, tree_iter_t *iter, int try_first )
{
	tree_t **sp = *psp;
	int any_tree = iter->search_id == prg->rtd->any_id;
	tree_t **top = iter->stack_root;
	kid_t *child;

	if ( try_first ) {
		while ( true ) {
			if ( top == vm_ptop() || iter->ref.kid->next == 0 ) {
				child = tree_child( prg, iter->ref.kid->tree );

				if ( child == 0 )
					break;
				vm_contiguous( 2 );
				vm_push_ref( iter->ref.next );
				vm_push_kid( iter->ref.kid );
				iter->ref.kid = child;
				iter->ref.next = (ref_t*)vm_ptop();
			}
			else {
				/* Not the top and not there is a next, go over to it. */
				iter->ref.kid = iter->ref.kid->next;
			}
		}

		goto first;
	}

	while ( true ) {
		if ( top == vm_ptop() ) {
			iter->ref.kid = 0;
			return;
		}
		
		if ( iter->ref.kid->next == 0 ) {
			/* Go up one and then down. Remember we can't use iter->ref.next
			 * because the chain may have been split, setting it null (to
			 * prevent repeated walks up). */
			ref_t *ref = (ref_t*)vm_ptop();
			iter->ref.kid = tree_child( prg, ref->kid->tree );
		}
		else {
			iter->ref.kid = vm_pop_kid();
			iter->ref.next = vm_pop_ref();
		}
first:
		if ( iter->ref.kid->tree->id == iter->search_id || any_tree ) {
			*psp = sp;
			return;
		}
	}
	*psp = sp;
	return;
}


tree_t *tree_iter_prev_repeat( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yield_size == (vm_ssize() - iter->root_size) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		iter->ref = iter->root_ref;
		iter_find_rev_repeat( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iter_find_rev_repeat( prg, psp, iter, false );
	}

	sp = *psp;
	iter->yield_size = vm_ssize() - iter->root_size;

	return (iter->ref.kid ? prg->true_val : prg->false_val );
}



