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

#define true 1
#define false 0

void initTreeIter( TreeIter *treeIter, Tree **stackRoot, long rootSize,
		const Ref *rootRef, int searchId )
{
	treeIter->type = IT_Tree;
	treeIter->rootRef = *rootRef;
	treeIter->searchId = searchId;
	treeIter->stackRoot = stackRoot;
	treeIter->yieldSize = 0;
	treeIter->rootSize = rootSize;
	treeIter->ref.kid = 0;
	treeIter->ref.next = 0;
}

void initRevTreeIter( RevTreeIter *revTriter, Tree **stackRoot, long rootSize,
		const Ref *rootRef, int searchId, int children )
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
}

void initUserIter( UserIter *userIter, Tree **stackRoot, long rootSize,
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


UserIter *uiterCreate( Program *prg, Tree ***psp, FunctionInfo *fi, long searchId )
{
	Tree **sp = *psp;

	vm_pushn( sizeof(UserIter) / sizeof(Word) );
	void *mem = vm_ptop();
	UserIter *uiter = mem;

	Tree **stackRoot = vm_ptop();
	long rootSize = vm_ssize();

	initUserIter( uiter, stackRoot, rootSize, fi->argSize, searchId );

	*psp = sp;
	return uiter;
}

void uiterInit( Program *prg, Tree **sp, UserIter *uiter, 
		FunctionInfo *fi, int revertOn )
{
	/* Set up the first yeild so when we resume it starts at the beginning. */
	uiter->ref.kid = 0;
	uiter->yieldSize = vm_ssize() - uiter->rootSize;
	uiter->frame = &uiter->stackRoot[-IFR_AA];

	if ( revertOn )
		uiter->resume = prg->rtd->frameInfo[fi->frameId].codeWV;
	else
		uiter->resume = prg->rtd->frameInfo[fi->frameId].codeWC;
}


void treeIterDestroy( Program *prg, Tree ***psp, TreeIter *iter )
{
	if ( (int)iter->type != 0 ) {
		Tree **sp = *psp;
		long curStackSize = vm_ssize() - iter->rootSize;
		assert( iter->yieldSize == curStackSize );
		vm_popn( iter->yieldSize );
		iter->type = 0;
		*psp = sp;
	}
}

void revTreeIterDestroy( struct colm_program *prg, Tree ***psp, RevTreeIter *riter )
{
	if ( (int)riter->type != 0 ) {
		Tree **sp = *psp;
		long curStackSize = vm_ssize() - riter->rootSize;
		assert( riter->yieldSize == curStackSize );
		vm_popn( riter->yieldSize );
		riter->type = 0;
		*psp = sp;
	}
}

void userIterDestroy( Program *prg, Tree ***psp, UserIter *uiter )
{
	if ( uiter != 0 && (int)uiter->type != 0 ) {
		Tree **sp = *psp;

		/* We should always be coming from a yield. The current stack size will be
		 * nonzero and the stack size in the iterator will be correct. */
		long curStackSize = vm_ssize() - uiter->rootSize;
		assert( uiter->yieldSize == curStackSize );

		long argSize = uiter->argSize;

		vm_popn( uiter->yieldSize );
		vm_popn( sizeof(UserIter) / sizeof(Word) );
		vm_popn( argSize );

		uiter->type = 0;

		*psp = sp;
	}
}

void userIterDestroy2( Program *prg, Tree ***psp, UserIter *uiter )
{
	if ( uiter != 0 && (int)uiter->type != 0 ) {
		Tree **sp = *psp;

		/* We should always be coming from a yield. The current stack size will be
		 * nonzero and the stack size in the iterator will be correct. */
		long curStackSize = vm_ssize() - uiter->rootSize;
		assert( uiter->yieldSize == curStackSize );

		long argSize = uiter->argSize;

		vm_popn( uiter->yieldSize );
		vm_popn( sizeof(UserIter) / sizeof(Word) );
		vm_popn( argSize );

		uiter->type = 0;

		*psp = sp;
	}
}

Tree *treeIterDerefCur( TreeIter *iter )
{
	return iter->ref.kid == 0 ? 0 : iter->ref.kid->tree;
}

void setTriterCur( Program *prg, TreeIter *iter, Tree *tree )
{
	iter->ref.kid->tree = tree;
}

void setUiterCur( Program *prg, UserIter *uiter, Tree *tree )
{
	uiter->ref.kid->tree = tree;
}

void splitIterCur( Program *prg, Tree ***psp, TreeIter *iter )
{
	if ( iter->ref.kid == 0 )
		return;
	
	splitRef( prg, psp, &iter->ref );
}

void iterFind( Program *prg, Tree ***psp, TreeIter *iter, int tryFirst )
{
	int anyTree = iter->searchId == prg->rtd->anyId;
	Tree **top = iter->stackRoot;
	Kid *child;
	Tree **sp = *psp;

rec_call:
	if ( tryFirst && ( iter->ref.kid->tree->id == iter->searchId || anyTree ) ) {
		*psp = sp;
		return;
	}
	else {
		child = treeChild( prg, iter->ref.kid->tree );
		if ( child != 0 ) {
			vm_contiguous( 2 );
			vm_push( (SW) iter->ref.next );
			vm_push( (SW) iter->ref.kid );
			iter->ref.kid = child;
			iter->ref.next = (Ref*)vm_ptop();
			while ( iter->ref.kid != 0 ) {
				tryFirst = true;
				goto rec_call;
				rec_return:
				iter->ref.kid = iter->ref.kid->next;
			}
			iter->ref.kid = (Kid*)vm_pop();
			iter->ref.next = (Ref*)vm_pop();
		}
	}

	if ( top != vm_ptop() )
		goto rec_return;
	
	iter->ref.kid = 0;
	*psp = sp;
}

Tree *treeIterAdvance( Program *prg, Tree ***psp, TreeIter *iter )
{
	Tree **sp = *psp;
	assert( iter->yieldSize == (vm_ssize() - iter->rootSize) );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the root. */
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

Tree *treeIterNextChild( Program *prg, Tree ***psp, TreeIter *iter )
{
	Tree **sp = *psp;
	assert( iter->yieldSize == (vm_ssize() - iter->rootSize) );
	Kid *kid = 0;

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the first child. */
		Kid *child = treeChild( prg, iter->rootRef.kid->tree );

		if ( child == 0 )
			iter->ref.next = 0;
		else {
			/* Make a reference to the root. */
			vm_contiguous( 2 );
			vm_push( (SW) iter->rootRef.next );
			vm_push( (SW) iter->rootRef.kid );
			iter->ref.next = (Ref*)vm_ptop();

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

Tree *treeRevIterPrevChild( Program *prg, Tree ***psp, RevTreeIter *iter )
{
	Tree **sp = *psp;
	assert( iter->yieldSize == ( vm_ssize() - iter->rootSize ) );

	if ( iter->kidAtYield != iter->ref.kid ) {
		/* Need to reload the kids. */
		vm_popn( iter->children );

		int c;
		Kid *kid = treeChild( prg, iter->rootRef.kid->tree );
		for ( c = 0; c < iter->children; c++ ) {
			vm_push( (SW)kid );
			kid = kid->next;
		}
	}

	if ( iter->ref.kid != 0 ) {
		vm_pop_ignore();
		iter->children -= 1;
	}

	if ( iter->searchId != prg->rtd->anyId ) {
		/* Have a previous item, go to the next sibling. */
		while ( iter->children > 0 && ((Kid*)(vm_top()))->tree->id != iter->searchId ) {
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
		iter->ref.kid = (Kid*)vm_top();
	}

	/* We will use this to detect a split above the iterated tree. */
	iter->kidAtYield = iter->ref.kid;

	iter->yieldSize = vm_ssize() - iter->rootSize;

	*psp = sp;

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

void iterFindRepeat( Program *prg, Tree ***psp, TreeIter *iter, int tryFirst )
{
	Tree **sp = *psp;
	int anyTree = iter->searchId == prg->rtd->anyId;
	Tree **top = iter->stackRoot;
	Kid *child;

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
				vm_push( (SW) iter->ref.next );
				vm_push( (SW) iter->ref.kid );
				iter->ref.kid = child;
				iter->ref.next = (Ref*)vm_ptop();
				while ( iter->ref.kid != 0 ) {
					tryFirst = true;
					goto rec_call;
					rec_return:
					iter->ref.kid = iter->ref.kid->next;
				}
				iter->ref.kid = (Kid*)vm_pop();
				iter->ref.next = (Ref*)vm_pop();
			}
		}
	}

	if ( top != vm_ptop() )
		goto rec_return;
	
	iter->ref.kid = 0;
	*psp = sp;
}

Tree *treeIterNextRepeat( Program *prg, Tree ***psp, TreeIter *iter )
{
	Tree **sp = *psp;
	assert( iter->yieldSize == ( vm_ssize() - iter->rootSize ) );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the root. */
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

void iterFindRevRepeat( Program *prg, Tree ***psp, TreeIter *iter, int tryFirst )
{
	Tree **sp = *psp;
	int anyTree = iter->searchId == prg->rtd->anyId;
	Tree **top = iter->stackRoot;
	Kid *child;

	if ( tryFirst ) {
		while ( true ) {
			if ( top == vm_ptop() || iter->ref.kid->next == 0 ) {
				child = treeChild( prg, iter->ref.kid->tree );

				if ( child == 0 )
					break;
				vm_contiguous( 2 );
				vm_push( (SW) iter->ref.next );
				vm_push( (SW) iter->ref.kid );
				iter->ref.kid = child;
				iter->ref.next = (Ref*)vm_ptop();
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
			Ref *ref = (Ref*)vm_ptop();
			iter->ref.kid = treeChild( prg, ref->kid->tree );
		}
		else {
			iter->ref.kid = (Kid*)vm_pop();
			iter->ref.next = (Ref*)vm_pop();
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


Tree *treeIterPrevRepeat( Program *prg, Tree ***psp, TreeIter *iter )
{
	Tree **sp = *psp;
	assert( iter->yieldSize == (vm_ssize() - iter->rootSize) );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the root. */
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



