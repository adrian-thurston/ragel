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

void colm_init_list_iter( generic_iter_t *listIter, tree_t **stackRoot,
		long argSize, long rootSize, const ref_t *rootRef, int genericId )
{
	listIter->type = IT_Tree;
	listIter->rootRef = *rootRef;
	listIter->stackRoot = stackRoot;
	listIter->yieldSize = 0;
	listIter->rootSize = rootSize;
	listIter->ref.kid = 0;
	listIter->ref.next = 0;
	listIter->argSize = argSize;
	listIter->genericId = genericId;
}

void colm_list_iter_destroy( program_t *prg, tree_t ***psp, generic_iter_t *iter )
{
	if ( (int)iter->type != 0 ) {
		int i;
		tree_t **sp = *psp;
		long curStackSize = vm_ssize() - iter->rootSize;
		assert( iter->yieldSize == curStackSize );
		vm_popn( iter->yieldSize );
		for ( i = 0; i < iter->argSize; i++ )
			colm_tree_downref( prg, sp, vm_pop_tree() );
		iter->type = 0;
		*psp = sp;
	}
}

tree_t *colm_list_iter_advance( program_t *prg, tree_t ***psp, generic_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yieldSize == (vm_ssize() - iter->rootSize) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		list_t *list = *((list_t**)iter->rootRef.kid);
		iter->ref.kid = (kid_t*)list->head;
		iter->ref.next = 0;

		//= iter->rootRef;
		//iter
		//iterFind( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		//iterFind( prg, psp, iter, false );

		list_el_t *listEl = (list_el_t*)iter->ref.kid;
		listEl = listEl->list_next;
		iter->ref.kid = (kid_t*)listEl;
		iter->ref.next = 0;
	}

	sp = *psp;
	iter->yieldSize = vm_ssize() - iter->rootSize;

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

tree_t *colm_map_iter_advance( program_t *prg, tree_t ***psp, generic_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yieldSize == (vm_ssize() - iter->rootSize) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		map_t *map = *((map_t**)iter->rootRef.kid);
		iter->ref.kid = (kid_t*)map->head;
		iter->ref.next = 0;

		//= iter->rootRef;
		//iter
		//iterFind( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		//iterFind( prg, psp, iter, false );

		map_el_t *mapEl = (map_el_t*)iter->ref.kid;
		mapEl = mapEl->next;
		iter->ref.kid = (kid_t*)mapEl;
		iter->ref.next = 0;
	}

	sp = *psp;
	iter->yieldSize = vm_ssize() - iter->rootSize;

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

tree_t *colm_list_iter_deref_cur( program_t *prg, generic_iter_t *iter )
{
	struct generic_info *gi = &prg->rtd->genericInfo[iter->genericId];
	list_el_t *el = (list_el_t*)iter->ref.kid;
	struct colm_struct *s = el != 0 ?
			colm_struct_container( el, gi->elOffset ) : 0;
	return (tree_t*)s;
}

value_t colm_viter_deref_cur( program_t *prg, generic_iter_t *iter )
{
	struct generic_info *gi = &prg->rtd->genericInfo[iter->genericId];
	list_el_t *el = (list_el_t*)iter->ref.kid;
	struct colm_struct *s = el != 0 ?
			colm_struct_container( el, gi->elOffset ) : 0;

	value_t value = colm_struct_get_field( s, value_t, 0 );
	if ( gi->valueType == TYPE_TREE )
		colm_tree_upref( (tree_t*)value );

	return value;
}

void colm_init_tree_iter( tree_iter_t *treeIter, tree_t **stackRoot,
		long argSize, long rootSize,
		const ref_t *rootRef, int searchId )
{
	treeIter->type = IT_Tree;
	treeIter->rootRef = *rootRef;
	treeIter->searchId = searchId;
	treeIter->stackRoot = stackRoot;
	treeIter->yieldSize = 0;
	treeIter->rootSize = rootSize;
	treeIter->ref.kid = 0;
	treeIter->ref.next = 0;
	treeIter->argSize = argSize;
}

void colm_init_rev_tree_iter( rev_tree_iter_t *revTriter, tree_t **stackRoot,
		long argSize, long rootSize,
		const ref_t *rootRef, int searchId, int children )
{
	revTriter->type = IT_RevTree;
	revTriter->rootRef = *rootRef;
	revTriter->searchId = searchId;
	revTriter->stackRoot = stackRoot;
	revTriter->yieldSize = children;
	revTriter->rootSize = rootSize;
	revTriter->kidAtYield = 0;
	revTriter->children = children;
	revTriter->ref.kid = 0;
	revTriter->ref.next = 0;
	revTriter->argSize = argSize;
}

void initUserIter( user_iter_t *userIter, tree_t **stackRoot, long rootSize,
		long argSize, long searchId )
{
	userIter->type = IT_User;
	userIter->stackRoot = stackRoot;
	userIter->argSize = argSize;
	userIter->yieldSize = 0;
	userIter->rootSize = rootSize;
	userIter->resume = 0;
	userIter->frame = 0;
	userIter->searchId = searchId;

	userIter->ref.kid = 0;
	userIter->ref.next = 0;
}


user_iter_t *colm_uiter_create( program_t *prg, tree_t ***psp, struct function_info *fi, long searchId )
{
	tree_t **sp = *psp;

	vm_pushn( sizeof(user_iter_t) / sizeof(word_t) );
	void *mem = vm_ptop();
	user_iter_t *uiter = mem;

	tree_t **stackRoot = vm_ptop();
	long rootSize = vm_ssize();

	initUserIter( uiter, stackRoot, rootSize, fi->argSize, searchId );

	*psp = sp;
	return uiter;
}

void uiterInit( program_t *prg, tree_t **sp, user_iter_t *uiter, 
		struct function_info *fi, int revertOn )
{
	/* Set up the first yeild so when we resume it starts at the beginning. */
	uiter->ref.kid = 0;
	uiter->yieldSize = vm_ssize() - uiter->rootSize;
	//	uiter->frame = &uiter->stackRoot[-IFR_AA];

	if ( revertOn )
		uiter->resume = prg->rtd->frameInfo[fi->frameId].codeWV;
	else
		uiter->resume = prg->rtd->frameInfo[fi->frameId].codeWC;
}


void colm_tree_iter_destroy( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	if ( (int)iter->type != 0 ) {
		int i;
		tree_t **sp = *psp;
		long curStackSize = vm_ssize() - iter->rootSize;
		assert( iter->yieldSize == curStackSize );
		vm_popn( iter->yieldSize );
		for ( i = 0; i < iter->argSize; i++ )
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
		long curStackSize = vm_ssize() - riter->rootSize;
		assert( riter->yieldSize == curStackSize );
		vm_popn( riter->yieldSize );
		for ( i = 0; i < riter->argSize; i++ )
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
		long curStackSize = vm_ssize() - uiter->rootSize;
		assert( uiter->yieldSize == curStackSize );

		vm_popn( uiter->yieldSize );
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
		long curStackSize = vm_ssize() - uiter->rootSize;
		assert( uiter->yieldSize == curStackSize );

		long argSize = uiter->argSize;

		vm_popn( uiter->yieldSize );
		vm_popn( sizeof(user_iter_t) / sizeof(word_t) );

		/* The IN_PREP_ARGS stack data. */
		vm_popn( argSize );
		vm_pop_value();

		uiter->type = 0;

		*psp = sp;
	}
}

tree_t *treeIterDerefCur( tree_iter_t *iter )
{
	return iter->ref.kid == 0 ? 0 : iter->ref.kid->tree;
}

void setTriterCur( program_t *prg, tree_iter_t *iter, tree_t *tree )
{
	iter->ref.kid->tree = tree;
}

void setUiterCur( program_t *prg, user_iter_t *uiter, tree_t *tree )
{
	uiter->ref.kid->tree = tree;
}

void splitIterCur( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	if ( iter->ref.kid == 0 )
		return;
	
	splitRef( prg, psp, &iter->ref );
}

void iterFind( program_t *prg, tree_t ***psp, tree_iter_t *iter, int tryFirst )
{
	int anyTree = iter->searchId == prg->rtd->anyId;
	tree_t **top = iter->stackRoot;
	kid_t *child;
	tree_t **sp = *psp;

rec_call:
	if ( tryFirst && ( iter->ref.kid->tree->id == iter->searchId || anyTree ) ) {
		*psp = sp;
		return;
	}
	else {
		child = treeChild( prg, iter->ref.kid->tree );
		if ( child != 0 ) {
			vm_contiguous( 2 );
			vm_push_ref( iter->ref.next );
			vm_push_kid( iter->ref.kid );
			iter->ref.kid = child;
			iter->ref.next = (ref_t*)vm_ptop();
			while ( iter->ref.kid != 0 ) {
				tryFirst = true;
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

tree_t *treeIterAdvance( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yieldSize == (vm_ssize() - iter->rootSize) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		iter->ref = iter->rootRef;
		iterFind( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iterFind( prg, psp, iter, false );
	}

	sp = *psp;
	iter->yieldSize = vm_ssize() - iter->rootSize;

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

tree_t *treeIterNextChild( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yieldSize == (vm_ssize() - iter->rootSize) );
	kid_t *kid = 0;

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the first child. */
		kid_t *child = treeChild( prg, iter->rootRef.kid->tree );

		if ( child == 0 )
			iter->ref.next = 0;
		else {
			/* Make a reference to the root. */
			vm_contiguous( 2 );
			vm_push_ref( iter->rootRef.next );
			vm_push_kid( iter->rootRef.kid );
			iter->ref.next = (ref_t*)vm_ptop();

			kid = child;
		}
	}
	else {
		/* Start at next. */
		kid = iter->ref.kid->next;
	}

	if ( iter->searchId != prg->rtd->anyId ) {
		/* Have a previous item, go to the next sibling. */
		while ( kid != 0 && kid->tree->id != iter->searchId )
			kid = kid->next;
	}

	iter->ref.kid = kid;
	iter->yieldSize = vm_ssize() - iter->rootSize;
	*psp = sp;
	return ( iter->ref.kid ? prg->trueVal : prg->falseVal );
}

tree_t *treeRevIterPrevChild( program_t *prg, tree_t ***psp, rev_tree_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yieldSize == ( vm_ssize() - iter->rootSize ) );

	if ( iter->kidAtYield != iter->ref.kid ) {
		/* Need to reload the kids. */
		vm_popn( iter->children );

		int c;
		kid_t *kid = treeChild( prg, iter->rootRef.kid->tree );
		for ( c = 0; c < iter->children; c++ ) {
			vm_push_kid( kid );
			kid = kid->next;
		}
	}

	if ( iter->ref.kid != 0 ) {
		vm_pop_ignore();
		iter->children -= 1;
	}

	if ( iter->searchId != prg->rtd->anyId ) {
		/* Have a previous item, go to the next sibling. */
		while ( iter->children > 0 && ((kid_t*)(vm_top()))->tree->id != iter->searchId ) {
			iter->children -= 1;
			vm_pop_ignore();
		}
	}

	if ( iter->children == 0 ) {
		iter->ref.next = 0;
		iter->ref.kid = 0;
	}
	else {
		iter->ref.next = &iter->rootRef;
		iter->ref.kid = (kid_t*)vm_top();
	}

	/* We will use this to detect a split above the iterated tree. */
	iter->kidAtYield = iter->ref.kid;

	iter->yieldSize = vm_ssize() - iter->rootSize;

	*psp = sp;

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

void iterFindRepeat( program_t *prg, tree_t ***psp, tree_iter_t *iter, int tryFirst )
{
	tree_t **sp = *psp;
	int anyTree = iter->searchId == prg->rtd->anyId;
	tree_t **top = iter->stackRoot;
	kid_t *child;

rec_call:
	if ( tryFirst && ( iter->ref.kid->tree->id == iter->searchId || anyTree ) ) {
		*psp = sp;
		return;
	}
	else {
		/* The repeat iterator is just like the normal top-down-left-right,
		 * execept it only goes into the children of a node if the node is the
		 * root of the iteration, or if does not have any neighbours to the
		 * right. */
		if ( top == vm_ptop() || iter->ref.kid->next == 0  ) {
			child = treeChild( prg, iter->ref.kid->tree );
			if ( child != 0 ) {
				vm_contiguous( 2 );
				vm_push_ref( iter->ref.next );
				vm_push_kid( iter->ref.kid );
				iter->ref.kid = child;
				iter->ref.next = (ref_t*)vm_ptop();
				while ( iter->ref.kid != 0 ) {
					tryFirst = true;
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

tree_t *treeIterNextRepeat( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yieldSize == ( vm_ssize() - iter->rootSize ) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		iter->ref = iter->rootRef;
		iterFindRepeat( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iterFindRepeat( prg, psp, iter, false );
	}

	sp = *psp;
	iter->yieldSize = vm_ssize() - iter->rootSize;

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

void iterFindRevRepeat( program_t *prg, tree_t ***psp, tree_iter_t *iter, int tryFirst )
{
	tree_t **sp = *psp;
	int anyTree = iter->searchId == prg->rtd->anyId;
	tree_t **top = iter->stackRoot;
	kid_t *child;

	if ( tryFirst ) {
		while ( true ) {
			if ( top == vm_ptop() || iter->ref.kid->next == 0 ) {
				child = treeChild( prg, iter->ref.kid->tree );

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
			iter->ref.kid = treeChild( prg, ref->kid->tree );
		}
		else {
			iter->ref.kid = vm_pop_kid();
			iter->ref.next = vm_pop_ref();
		}
first:
		if ( iter->ref.kid->tree->id == iter->searchId || anyTree ) {
			*psp = sp;
			return;
		}
	}
	*psp = sp;
	return;
}


tree_t *treeIterPrevRepeat( program_t *prg, tree_t ***psp, tree_iter_t *iter )
{
	tree_t **sp = *psp;
	assert( iter->yieldSize == (vm_ssize() - iter->rootSize) );

	if ( iter->ref.kid == 0 ) {
		/* kid_t is zero, start from the root. */
		iter->ref = iter->rootRef;
		iterFindRevRepeat( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iterFindRevRepeat( prg, psp, iter, false );
	}

	sp = *psp;
	iter->yieldSize = vm_ssize() - iter->rootSize;

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}



