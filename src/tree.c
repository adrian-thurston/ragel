/*
 *  Copyright 2008-2012 Adrian Thurston <thurston@complang.org>
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#define true 1
#define false 0

#define BUFFER_INITIAL_SIZE 4096

void listPrepend( List *list, ListEl *new_el) { listAddBefore(list, list->head, new_el); }
void listAppend( List *list, ListEl *new_el)  { listAddAfter(list, list->tail, new_el); }

ListEl *listDetach( List *list, ListEl *el );
ListEl *listDetachFirst(List *list )        { return listDetach(list, list->head); }
ListEl *listDetachLast(List *list )         { return listDetach(list, list->tail); }

long listLength(List *list)
	{ return list->listLen; }

void initTreeIter( TreeIter *treeIter, Tree **stackRoot, long rootSize,
		const Ref *rootRef, int searchId )
{
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

Kid *allocAttrs( Program *prg, long length )
{
	Kid *cur = 0;
	long i;
	for ( i = 0; i < length; i++ ) {
		Kid *next = cur;
		cur = kidAllocate( prg );
		cur->next = next;
	}
	return cur;
}

void freeAttrs( Program *prg, Kid *attrs )
{
	Kid *cur = attrs;
	while ( cur != 0 ) {
		Kid *next = cur->next;
		kidFree( prg, cur );
		cur = next;
	}
}

void freeKidList( Program *prg, Kid *kid )
{
	while ( kid != 0 ) {
		Kid *next = kid->next;
		kidFree( prg, kid );
		kid = next;
	}
}

void setAttr( Tree *tree, long pos, Tree *val )
{
	long i;
	Kid *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	for ( i = 0; i < pos; i++ )
		kid = kid->next;
	kid->tree = val;
}

Tree *colm_get_global( Program *prg, long pos )
	{ return colm_get_attr( prg->global, pos ); }

Tree *colm_get_attr( Tree *tree, long pos )
{
	long i;
	Kid *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	for ( i = 0; i < pos; i++ )
		kid = kid->next;
	return kid->tree;
}


Tree *colm_get_repeat_next( Tree *tree )
{
	Kid *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	return kid->next->tree;
}

Tree *colm_get_repeat_val( Tree *tree )
{
	Kid *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;
	
	return kid->tree;
}

int colm_repeat_end( Tree *tree )
{
	Kid *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	return kid == 0;
}

int colm_list_last( Tree *tree )
{
	Kid *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	return kid->next == 0;
}

Kid *getAttrKid( Tree *tree, long pos )
{
	long i;
	Kid *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	for ( i = 0; i < pos; i++ )
		kid = kid->next;
	return kid;
}

Kid *kidListConcat( Kid *list1, Kid *list2 )
{
	if ( list1 == 0 )
		return list2;
	else if ( list2 == 0 )
		return list1;

	Kid *dest = list1;
	while ( dest->next != 0 )
		dest = dest->next;
	dest->next = list2;
	return list1;
}


Stream *openStreamFile( Program *prg, char *name, FILE *file )
{
	Stream *res = (Stream*)mapElAllocate( prg );
	res->id = LEL_ID_STREAM;
	res->in = newSourceStreamFile( name, file );
	return res;
}

Stream *openStreamFd( Program *prg, char *name, long fd )
{
	Stream *res = (Stream*)mapElAllocate( prg );
	res->id = LEL_ID_STREAM;
	res->in = newSourceStreamFd( name, fd );
	return res;
}

Stream *openFile( Program *prg, Tree *name, Tree *mode )
{
	Head *headName = ((Str*)name)->value;
	Head *headMode = ((Str*)mode)->value;
	Stream *stream = 0;

	const char *givenMode = stringData(headMode);
	const char *fopenMode = 0;
	if ( memcmp( givenMode, "r", stringLength(headMode) ) == 0 )
		fopenMode = "rb";
	else if ( memcmp( givenMode, "w", stringLength(headMode) ) == 0 )
		fopenMode = "wb";
	else {
		fatal( "unknown file open mode: %s\n", givenMode );
	}
	
	/* Need to make a C-string (null terminated). */
	char *fileName = (char*)malloc(stringLength(headName)+1);
	memcpy( fileName, stringData(headName), stringLength(headName) );
	fileName[stringLength(headName)] = 0;
	FILE *file = fopen( fileName, fopenMode );
	if ( file != 0 )
		stream = openStreamFile( prg, fileName, file );

	return stream;
}

Tree *constructInteger( Program *prg, long i )
{
	Int *integer = (Int*) treeAllocate( prg );
	integer->id = LEL_ID_INT;
	integer->value = i;

	return (Tree*)integer;
}

Tree *constructString( Program *prg, Head *s )
{
	Str *str = (Str*) treeAllocate( prg );
	str->id = LEL_ID_STR;
	str->value = s;

	return (Tree*)str;
}

Tree *constructPointer( Program *prg, Tree *tree )
{
	Kid *kid = kidAllocate( prg );
	kid->tree = tree;
	kid->next = prg->heap;
	prg->heap = kid;

	Pointer *pointer = (Pointer*) treeAllocate( prg );
	pointer->id = LEL_ID_PTR;
	pointer->value = kid;
	
	return (Tree*)pointer;
}

Tree *constructTerm( Program *prg, Word id, Head *tokdata )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;

	Tree *tree = treeAllocate( prg );
	tree->id = id;
	tree->refs = 0;
	tree->tokdata = tokdata;

	int objectLength = lelInfo[tree->id].objectLength;
	tree->child = allocAttrs( prg, objectLength );

	return tree;
}

Tree *constructStream( Program *prg )
{
	Stream *input = streamAllocate( prg );
	input->refs = 0;
	input->id = LEL_ID_STREAM;

	input->in = newSourceStreamGeneric( "<internal>" );
	return (Tree*)input;
}

Kid *constructReplacementKid( Tree **bindings, Program *prg, Kid *prev, long pat );

static Kid *constructIgnoreList( Program *prg, long ignoreInd )
{
	PatConsNode *nodes = prg->rtd->patReplNodes;

	Kid *first = 0, *last = 0;
	while ( ignoreInd >= 0 ) {
		Head *ignoreData = stringAllocPointer( prg, nodes[ignoreInd].data, nodes[ignoreInd].length );

		Tree *ignTree = treeAllocate( prg );
		ignTree->refs = 1;
		ignTree->id = nodes[ignoreInd].id;
		ignTree->tokdata = ignoreData;

		Kid *ignKid = kidAllocate( prg );
		ignKid->tree = ignTree;
		ignKid->next = 0;

		if ( last == 0 )
			first = ignKid;
		else
			last->next = ignKid;

		ignoreInd = nodes[ignoreInd].next;
		last = ignKid;
	}

	return first;
}

static Kid *constructLeftIgnoreList( Program *prg, long pat )
{
	PatConsNode *nodes = prg->rtd->patReplNodes;
	return constructIgnoreList( prg, nodes[pat].leftIgnore );
}

static Kid *constructRightIgnoreList( Program *prg, long pat )
{
	PatConsNode *nodes = prg->rtd->patReplNodes;
	return constructIgnoreList( prg, nodes[pat].rightIgnore );
}

static void insLeftIgnore( Program *prg, Tree *tree, Tree *ignoreList )
{
	assert( ! (tree->flags & AF_LEFT_IGNORE) );

	/* Allocate. */
	Kid *kid = kidAllocate( prg );
	kid->tree = ignoreList;
	treeUpref( ignoreList );

	/* Attach it. */
	kid->next = tree->child;
	tree->child = kid;

	tree->flags |= AF_LEFT_IGNORE;
}

static void insRightIgnore( Program *prg, Tree *tree, Tree *ignoreList )
{
	assert( ! (tree->flags & AF_RIGHT_IGNORE) );

	/* Insert an ignore head in the child list. */
	Kid *kid = kidAllocate( prg );
	kid->tree = ignoreList;
	treeUpref( ignoreList );

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

Tree *pushRightIgnore( Program *prg, Tree *pushTo, Tree *rightIgnore )
{
	/* About to alter the data tree. Split first. */
	pushTo = splitTree( prg, pushTo );

	if ( pushTo->flags & AF_RIGHT_IGNORE ) {
		/* The previous token already has a right ignore. Merge by
		 * attaching it as a left ignore of the new list. */
		Kid *curIgnore = treeRightIgnoreKid( prg, pushTo );
		insLeftIgnore( prg, rightIgnore, curIgnore->tree );

		/* Replace the current ignore. Safe to access refs here because we just
		 * upreffed it in insLeftIgnore. */
		curIgnore->tree->refs -= 1;
		curIgnore->tree = rightIgnore;
		treeUpref( rightIgnore );
	}
	else {
		/* Attach The ignore list. */
		insRightIgnore( prg, pushTo, rightIgnore );
	}

	return pushTo;
}

Tree *pushLeftIgnore( Program *prg, Tree *pushTo, Tree *leftIgnore )
{
	pushTo = splitTree( prg, pushTo );

	/* Attach as left ignore to the token we are sending. */
	if ( pushTo->flags & AF_LEFT_IGNORE ) {
		/* The token already has a left-ignore. Merge by attaching it as a
		 * right ignore of the new list. */
		Kid *curIgnore = treeLeftIgnoreKid( prg, pushTo );
		insRightIgnore( prg, leftIgnore, curIgnore->tree );

		/* Replace the current ignore. Safe to upref here because we just
		 * upreffed it in insRightIgnore. */
		curIgnore->tree->refs -= 1;
		curIgnore->tree = leftIgnore;
		treeUpref( leftIgnore );
	}
	else {
		/* Attach the ignore list. */
		insLeftIgnore( prg, pushTo, leftIgnore );
	}

	return pushTo;
}

static void remLeftIgnore( Program *prg, Tree **sp, Tree *tree )
{
	assert( tree->flags & AF_LEFT_IGNORE );

	Kid *next = tree->child->next;
	treeDownref( prg, sp, tree->child->tree );
	kidFree( prg, tree->child );
	tree->child = next;

	tree->flags &= ~AF_LEFT_IGNORE;
}

static void remRightIgnore( Program *prg, Tree **sp, Tree *tree )
{
	assert( tree->flags & AF_RIGHT_IGNORE );

	if ( tree->flags & AF_LEFT_IGNORE ) {
		Kid *next = tree->child->next->next;
		treeDownref( prg, sp, tree->child->next->tree );
		kidFree( prg, tree->child->next );
		tree->child->next = next;
	}
	else {
		Kid *next = tree->child->next;
		treeDownref( prg, sp, tree->child->tree );
		kidFree( prg, tree->child );
		tree->child = next;
	}

	tree->flags &= ~AF_RIGHT_IGNORE;
}

Tree *popRightIgnore( Program *prg, Tree **sp, Tree *popFrom, Tree **rightIgnore )
{
	/* Modifying the tree we are detaching from. */
	popFrom = splitTree( prg, popFrom );

	Kid *riKid = treeRightIgnoreKid( prg, popFrom );

	/* If the right ignore has a left ignore, then that was the original
	 * right ignore. */
	Kid *li = treeLeftIgnoreKid( prg, riKid->tree );
	if ( li != 0 ) {
		treeUpref( li->tree );
		remLeftIgnore( prg, sp, riKid->tree );
		*rightIgnore = riKid->tree;
		treeUpref( *rightIgnore );
		riKid->tree = li->tree;
	}
	else  {
		*rightIgnore = riKid->tree;
		treeUpref( *rightIgnore );
		remRightIgnore( prg, sp, popFrom );
	}

	return popFrom;
}

Tree *popLeftIgnore( Program *prg, Tree **sp, Tree *popFrom, Tree **leftIgnore )
{
	/* Modifying, make the write safe. */
	popFrom = splitTree( prg, popFrom );

	Kid *liKid = treeLeftIgnoreKid( prg, popFrom );

	/* If the left ignore has a right ignore, then that was the original
	 * left ignore. */
	Kid *ri = treeRightIgnoreKid( prg, liKid->tree );
	if ( ri != 0 ) {
		treeUpref( ri->tree );
		remRightIgnore( prg, sp, liKid->tree );
		*leftIgnore = liKid->tree;
		treeUpref( *leftIgnore );
		liKid->tree = ri->tree;
	}
	else {
		*leftIgnore = liKid->tree;
		treeUpref( *leftIgnore );
		remLeftIgnore( prg, sp, popFrom );
	}

	return popFrom;
}


/* Returns an uprefed tree. Saves us having to downref and bindings to zero to
 * return a zero-ref tree. */
Tree *constructReplacementTree( Kid *kid, Tree **bindings, Program *prg, long pat )
{
	PatConsNode *nodes = prg->rtd->patReplNodes;
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Tree *tree = 0;

	if ( nodes[pat].bindId > 0 ) {
		/* All bindings have been uprefed. */
		tree = bindings[nodes[pat].bindId];

		long ignore = nodes[pat].leftIgnore;
		Tree *leftIgnore = 0;
		if ( ignore >= 0 ) {
			Kid *ignore = constructLeftIgnoreList( prg, pat );

			leftIgnore = treeAllocate( prg );
			leftIgnore->id = LEL_ID_IGNORE;
			leftIgnore->child = ignore;

			tree = pushLeftIgnore( prg, tree, leftIgnore );
		}

		ignore = nodes[pat].rightIgnore;
		Tree *rightIgnore = 0;
		if ( ignore >= 0 ) {
			Kid *ignore = constructRightIgnoreList( prg, pat );

			rightIgnore = treeAllocate( prg );
			rightIgnore->id = LEL_ID_IGNORE;
			rightIgnore->child = ignore;

			tree = pushRightIgnore( prg, tree, rightIgnore );
		}
	}
	else {
		tree = treeAllocate( prg );
		tree->id = nodes[pat].id;
		tree->refs = 1;
		tree->tokdata = nodes[pat].length == 0 ? 0 :
				stringAllocPointer( prg, 
				nodes[pat].data, nodes[pat].length );
		tree->prodNum = nodes[pat].prodNum;

		int objectLength = lelInfo[tree->id].objectLength;

		Kid *attrs = allocAttrs( prg, objectLength );
		Kid *child = constructReplacementKid( bindings, prg, 
				0, nodes[pat].child );

		tree->child = kidListConcat( attrs, child );

		/* Right first, then left. */
		Kid *ignore = constructRightIgnoreList( prg, pat );
		if ( ignore != 0 ) {
			Tree *ignoreList = treeAllocate( prg );
			ignoreList->id = LEL_ID_IGNORE;
			ignoreList->refs = 1;
			ignoreList->child = ignore;

			Kid *ignoreHead = kidAllocate( prg );
			ignoreHead->tree = ignoreList;
			ignoreHead->next = tree->child;
			tree->child = ignoreHead;

			tree->flags |= AF_RIGHT_IGNORE;
		}

		ignore = constructLeftIgnoreList( prg, pat );
		if ( ignore != 0 ) {
			Tree *ignoreList = treeAllocate( prg );
			ignoreList->id = LEL_ID_IGNORE;
			ignoreList->refs = 1;
			ignoreList->child = ignore;

			Kid *ignoreHead = kidAllocate( prg );
			ignoreHead->tree = ignoreList;
			ignoreHead->next = tree->child;
			tree->child = ignoreHead;

			tree->flags |= AF_LEFT_IGNORE;
		}

		int i;
		for ( i = 0; i < lelInfo[tree->id].numCaptureAttr; i++ ) {
			long ci = pat+1+i;
			CaptureAttr *ca = prg->rtd->captureAttr + lelInfo[tree->id].captureAttr + i;
			Tree *attr = treeAllocate( prg );
			attr->id = nodes[ci].id;
			attr->refs = 1;
			attr->tokdata = nodes[ci].length == 0 ? 0 :
					stringAllocPointer( prg, 
					nodes[ci].data, nodes[ci].length );

			setAttr( tree, ca->offset, attr );
		}
	}

	return tree;
}

Kid *constructReplacementKid( Tree **bindings, Program *prg, Kid *prev, long pat )
{
	PatConsNode *nodes = prg->rtd->patReplNodes;
	Kid *kid = 0;

	if ( pat != -1 ) {
		kid = kidAllocate( prg );
		kid->tree = constructReplacementTree( kid, bindings, prg, pat );

		/* Recurse down next. */
		Kid *next = constructReplacementKid( bindings, prg, 
				kid, nodes[pat].next );

		kid->next = next;
	}

	return kid;
}

Tree *constructToken( Program *prg, Tree **root, long nargs )
{
	Tree **const sp = root;
	Tree **base = vm_ptop() + nargs;

	Int *idInt = (Int*)base[-1];
	Str *textStr = (Str*)base[-2];

	long id = idInt->value;
	Head *tokdata = stringCopy( prg, textStr->value );

	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Tree *tree;

	if ( lelInfo[id].ignore ) {
		tree = treeAllocate( prg );
		tree->refs = 1;
		tree->id = id;
		tree->tokdata = tokdata;
	}
	else {
		long objectLength = lelInfo[id].objectLength;
		Kid *attrs = allocAttrs( prg, objectLength );

		tree = treeAllocate( prg );
		tree->id = id;
		tree->refs = 1;
		tree->tokdata = tokdata;

		tree->child = attrs;

		assert( nargs-2 <= objectLength );
		long id;
		for ( id = 0; id < nargs-2; id++ ) {
			setAttr( tree, id, base[-3-id] );
			treeUpref( colm_get_attr( tree, id) );
		}
	}
	return tree;
}

Tree *castTree( Program *prg, int langElId, Tree *tree )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;

	/* Need to keep a lookout for next down. If 
	 * copying it, return the copy. */
	Tree *newTree = treeAllocate( prg );

	newTree->id = langElId;
	newTree->tokdata = stringCopy( prg, tree->tokdata );

	/* Invalidate the production number. */
	newTree->prodNum = -1;

	/* Copy the child list. Start with ignores, then the list. */
	Kid *child = tree->child, *last = 0;

	/* Flags we are interested in. */
	newTree->flags |= tree->flags & ( AF_LEFT_IGNORE | AF_RIGHT_IGNORE );

	int ignores = 0;
	if ( tree->flags & AF_LEFT_IGNORE )
		ignores += 1;
	if ( tree->flags & AF_RIGHT_IGNORE )
		ignores += 1;

	/* Igores. */
	while ( ignores-- > 0 ) {
		Kid *newKid = kidAllocate( prg );

		newKid->tree = child->tree;
		newKid->next = 0;
		newKid->tree->refs += 1;

		/* Store the first child. */
		if ( last == 0 )
			newTree->child = newKid;
		else
			last->next = newKid;

		child = child->next;
		last = newKid;
	}

	/* Skip over the source's attributes. */
	int objectLength = lelInfo[tree->id].objectLength;
	while ( objectLength-- > 0 )
		child = child->next;

	/* Allocate the target type's kids. */
	objectLength = lelInfo[langElId].objectLength;
	while ( objectLength-- > 0 ) {
		Kid *newKid = kidAllocate( prg );

		newKid->tree = 0;
		newKid->next = 0;

		/* Store the first child. */
		if ( last == 0 )
			newTree->child = newKid;
		else
			last->next = newKid;

		last = newKid;
	}
	
	/* Copy the source's children. */
	while ( child != 0 ) {
		Kid *newKid = kidAllocate( prg );

		newKid->tree = child->tree;
		newKid->next = 0;
		newKid->tree->refs += 1;

		/* Store the first child. */
		if ( last == 0 )
			newTree->child = newKid;
		else
			last->next = newKid;

		child = child->next;
		last = newKid;
	}
	
	return newTree;
}

Tree *makeTree( Program *prg, Tree **root, long nargs )
{
	Tree **const sp = root;
	Tree **base = vm_ptop() + nargs;

	Int *idInt = (Int*)base[-1];

	long id = idInt->value;
	LangElInfo *lelInfo = prg->rtd->lelInfo;

	Tree *tree = treeAllocate( prg );
	tree->id = id;
	tree->refs = 1;

	long objectLength = lelInfo[id].objectLength;
	Kid *attrs = allocAttrs( prg, objectLength );

	Kid *last = 0, *child = 0;
	for ( id = 0; id < nargs-1; id++ ) {
		Kid *kid = kidAllocate( prg );
		kid->tree = base[-2-id];
		treeUpref( kid->tree );

		if ( last == 0 )
			child = kid;
		else
			last->next = kid;

		last = kid;
	}

	tree->child = kidListConcat( attrs, child );

	return tree;
}

int testFalse( Program *prg, Tree *tree )
{
	int flse = ( 
		tree == 0 ||
		tree == prg->falseVal ||
		( tree->id == LEL_ID_INT && ((Int*)tree)->value == 0 ) );
	return flse;
}

Kid *copyIgnoreList( Program *prg, Kid *ignoreHeader )
{
	Kid *newHeader = kidAllocate( prg );
	Kid *last = 0, *ic = (Kid*)ignoreHeader->tree;
	while ( ic != 0 ) {
		Kid *newIc = kidAllocate( prg );

		newIc->tree = ic->tree;
		newIc->tree->refs += 1;

		/* List pointers. */
		if ( last == 0 )
			newHeader->tree = (Tree*)newIc;
		else
			last->next = newIc;

		ic = ic->next;
		last = newIc;
	}
	return newHeader;
}

Kid *copyKidList( Program *prg, Kid *kidList )
{
	Kid *newList = 0, *last = 0, *ic = kidList;

	while ( ic != 0 ) {
		Kid *newIc = kidAllocate( prg );

		newIc->tree = ic->tree;
		treeUpref( newIc->tree );

		/* List pointers. */
		if ( last == 0 )
			newList = newIc;
		else
			last->next = newIc;

		ic = ic->next;
		last = newIc;
	}
	return newList;
}

/* New tree has zero ref. */
Tree *copyRealTree( Program *prg, Tree *tree, Kid *oldNextDown, Kid **newNextDown )
{
	/* Need to keep a lookout for next down. If 
	 * copying it, return the copy. */
	Tree *newTree = treeAllocate( prg );

	newTree->id = tree->id;
	newTree->tokdata = stringCopy( prg, tree->tokdata );
	newTree->prodNum = tree->prodNum;

	/* Copy the child list. Start with ignores, then the list. */
	Kid *child = tree->child, *last = 0;

	/* Left ignores. */
	if ( tree->flags & AF_LEFT_IGNORE ) {
		newTree->flags |= AF_LEFT_IGNORE;
//		Kid *newHeader = copyIgnoreList( prg, child );
//
//		/* Always the head. */
//		newTree->child = newHeader;
//
//		child = child->next;
//		last = newHeader;
	}

	/* Right ignores. */
	if ( tree->flags & AF_RIGHT_IGNORE ) {
		newTree->flags |= AF_RIGHT_IGNORE;
//		Kid *newHeader = copyIgnoreList( prg, child );
//		if ( last == 0 )
//			newTree->child = newHeader;
//		else
//			last->next = newHeader;
//		child = child->next;
//		last = newHeader;
	}

	/* Attributes and children. */
	while ( child != 0 ) {
		Kid *newKid = kidAllocate( prg );

		/* Watch out for next down. */
		if ( child == oldNextDown )
			*newNextDown = newKid;

		newKid->tree = child->tree;
		newKid->next = 0;

		/* May be an attribute. */
		if ( newKid->tree != 0 )
			newKid->tree->refs += 1;

		/* Store the first child. */
		if ( last == 0 )
			newTree->child = newKid;
		else
			last->next = newKid;

		child = child->next;
		last = newKid;
	}
	
	return newTree;
}

List *copyList( Program *prg, List *list, Kid *oldNextDown, Kid **newNextDown )
{
	/* Not a need copy. */
	List *newList = (List*)mapElAllocate( prg );
	newList->id = list->genericInfo->langElId;
	newList->genericInfo = list->genericInfo;

	ListEl *src = list->head;
	while( src != 0 ) {
		ListEl *newEl = listElAllocate( prg );
		newEl->value = src->value;
		treeUpref( newEl->value );

		listAppend( newList, newEl );

		/* Watch out for next down. */
		if ( (Kid*)src == oldNextDown )
			*newNextDown = (Kid*)newEl;

		src = src->next;
	}

	return newList;
}
	
Map *copyMap( Program *prg, Map *map, Kid *oldNextDown, Kid **newNextDown )
{
	Map *newMap = (Map*)mapElAllocate( prg );
	newMap->id = map->genericInfo->langElId;
	newMap->genericInfo = map->genericInfo;
	newMap->treeSize = map->treeSize;
	newMap->root = 0;

	/* If there is a root, copy the tree. */
	if ( map->root != 0 ) {
		newMap->root = mapCopyBranch( prg, newMap, map->root, 
				oldNextDown, newNextDown );
	}
	MapEl *el;
	for ( el = newMap->head; el != 0; el = el->next ) {
		assert( map->genericInfo->typeArg == TYPE_TREE );
		treeUpref( el->tree );
	}

	return newMap;
}

Tree *copyTree( Program *prg, Tree *tree, Kid *oldNextDown, Kid **newNextDown )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	long genericId = lelInfo[tree->id].genericId;
	if ( genericId > 0 ) {
		GenericInfo *generic = &prg->rtd->genericInfo[genericId];
		if ( generic->type == GEN_LIST )
			tree = (Tree*) copyList( prg, (List*) tree, oldNextDown, newNextDown );
		else if ( generic->type == GEN_MAP )
			tree = (Tree*) copyMap( prg, (Map*) tree, oldNextDown, newNextDown );
		else if ( generic->type == GEN_PARSER ) {
			/* Need to figure out the semantics here. */
			fatal( "ATTEMPT TO COPY PARSER\n" );
			assert(false);
		}
	}
	else if ( tree->id == LEL_ID_PTR )
		assert(false);
	else if ( tree->id == LEL_ID_BOOL )
		assert(false);
	else if ( tree->id == LEL_ID_INT )
		assert(false);
	else if ( tree->id == LEL_ID_STR )
		assert(false);
	else if ( tree->id == LEL_ID_STREAM )
		assert(false);
	else {
		tree = copyRealTree( prg, tree, oldNextDown, newNextDown );
	}

	assert( tree->refs == 0 );
	return tree;
}

Tree *splitTree( Program *prg, Tree *tree )
{
	if ( tree != 0 ) {
		assert( tree->refs >= 1 );

		if ( tree->refs > 1 ) {
			Kid *oldNextDown = 0, *newNextDown = 0;
			Tree *newTree = copyTree( prg, tree, oldNextDown, &newNextDown );
			treeUpref( newTree );

			/* Downref the original. Don't need to consider freeing because
			 * refs were > 1. */
			tree->refs -= 1;

			tree = newTree;
		}

		assert( tree->refs == 1 );
	}
	return tree;
}

Tree *createGeneric( Program *prg, long genericId )
{
	GenericInfo *genericInfo = &prg->rtd->genericInfo[genericId];
	Tree *newGeneric = 0;
	switch ( genericInfo->type ) {
		case GEN_MAP: {
			Map *map = (Map*)mapElAllocate( prg );
			map->id = genericInfo->langElId;
			map->genericInfo = genericInfo;
			newGeneric = (Tree*) map;
			break;
		}
		case GEN_LIST: {
			List *list = (List*)mapElAllocate( prg );
			list->id = genericInfo->langElId;
			list->genericInfo = genericInfo;
			newGeneric = (Tree*) list;
			break;
		}
		case GEN_PARSER: {
			Parser *parser = (Parser*)mapElAllocate( prg );
			parser->id = genericInfo->langElId;
			parser->genericInfo = genericInfo;
			parser->pdaRun = malloc( sizeof(PdaRun) );

			/* Start off the parsing process. */
			initPdaRun( prg, parser->pdaRun, prg->rtd->pdaTables, 
					genericInfo->parserId, false, false, 0 );

			newGeneric = (Tree*) parser;
			break;
		}
		default:
			assert(false);
			return 0;
	}

	return newGeneric;
}


/* We can't make recursive calls here since the tree we are freeing may be
 * very large. Need the VM stack. */
void treeFreeRec( Program *prg, Tree **sp, Tree *tree )
{
	Tree **top = vm_ptop();
	LangElInfo *lelInfo;
	long genericId;

free_tree:
	lelInfo = prg->rtd->lelInfo;
	genericId = lelInfo[tree->id].genericId;
	if ( genericId > 0 ) {
		GenericInfo *generic = &prg->rtd->genericInfo[genericId];
		if ( generic->type == GEN_LIST ) {
			List *list = (List*) tree;
			ListEl *el = list->head;
			while ( el != 0 ) {
				ListEl *next = el->next;
				vm_push( el->value );
				listElFree( prg, el );
				el = next;
			}
			mapElFree( prg, (MapEl*)list );
		}
		else if ( generic->type == GEN_MAP ) {
			Map *map = (Map*)tree;
			MapEl *el = map->head;
			while ( el != 0 ) {
				MapEl *next = el->next;
				vm_push( el->key );
				vm_push( el->tree );
				mapElFree( prg, el );
				el = next;
			}
			mapElFree( prg, (MapEl*)map );
		}
		else if ( generic->type == GEN_PARSER ) {
			Parser *parser = (Parser*)tree;
			clearPdaRun( prg, sp, parser->pdaRun );
			free( parser->pdaRun );
			treeDownref( prg, sp, (Tree*)parser->input );
			mapElFree( prg, (MapEl*)parser );
		}
		else {
			assert(false);
		}
	}
	else {
		if ( tree->id == LEL_ID_STR ) {
			Str *str = (Str*) tree;
			stringFree( prg, str->value );
			treeFree( prg, tree );
		}
		else if ( tree->id == LEL_ID_BOOL || tree->id == LEL_ID_INT )
			treeFree( prg, tree );
		else if ( tree->id == LEL_ID_PTR )
			treeFree( prg, tree );
		else if ( tree->id == LEL_ID_STREAM ) {
			Stream *stream = (Stream*)tree;
			clearSourceStream( prg, sp, stream->in );
			if ( stream->in->file != 0 )
				fclose( stream->in->file );
			else if ( stream->in->fd > 0 )
				close( stream->in->fd );
			free( stream->in );
			streamFree( prg, stream );
		}
		else { 
			if ( tree->id != LEL_ID_IGNORE )
				stringFree( prg, tree->tokdata );

			/* Attributes and grammar-based children. */
			Kid *child = tree->child;
			while ( child != 0 ) {
				Kid *next = child->next;
				vm_push( child->tree );
				kidFree( prg, child );
				child = next;
			}

			treeFree( prg, tree );
		}
	}

	/* Any trees to downref? */
	while ( sp != top ) {
		tree = vm_pop();
		if ( tree != 0 ) {
			assert( tree->refs > 0 );
			tree->refs -= 1;
			if ( tree->refs == 0 )
				goto free_tree;
		}
	}
}

void treeUpref( Tree *tree )
{
	if ( tree != 0 )
		tree->refs += 1;
}

void treeDownref( Program *prg, Tree **sp, Tree *tree )
{
	if ( tree != 0 ) {
		assert( tree->refs > 0 );
		tree->refs -= 1;
		if ( tree->refs == 0 )
			treeFreeRec( prg, sp, tree );
	}
}

/* Find the first child of a tree. */
Kid *treeChild( Program *prg, const Tree *tree )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Kid *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	/* Skip over attributes. */
	long objectLength = lelInfo[tree->id].objectLength;
	long a;
	for ( a = 0; a < objectLength; a++ )
		kid = kid->next;

	return kid;
}

/* Detach at the first real child of a tree. */
Kid *treeExtractChild( Program *prg, Tree *tree )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Kid *kid = tree->child, *last = 0;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	/* Skip over attributes. */
	long a, objectLength = lelInfo[tree->id].objectLength;
	for ( a = 0; a < objectLength; a++ ) {
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
Kid *treeAttr( Program *prg, const Tree *tree )
{
	Kid *kid = tree->child;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	return kid;
}

Tree *treeLeftIgnore( Program *prg, Tree *tree )
{
	if ( tree->flags & AF_LEFT_IGNORE )
		return tree->child->tree;
	return 0;
}

Tree *treeRightIgnore( Program *prg, Tree *tree )
{
	if ( tree->flags & AF_RIGHT_IGNORE ) {
		if ( tree->flags & AF_LEFT_IGNORE )
			return tree->child->next->tree;
		else
			return tree->child->tree;
	}
	return 0;
}

Kid *treeLeftIgnoreKid( Program *prg, Tree *tree )
{
	if ( tree->flags & AF_LEFT_IGNORE )
		return tree->child;
	return 0;
}

Kid *treeRightIgnoreKid( Program *prg, Tree *tree )
{
	if ( tree->flags & AF_RIGHT_IGNORE ) {
		if ( tree->flags & AF_LEFT_IGNORE )
			return tree->child->next;
		else
			return tree->child;
	}
	return 0;
}

Tree *treeIterDerefCur( TreeIter *iter )
{
	return iter->ref.kid == 0 ? 0 : iter->ref.kid->tree;
}

void refSetValue( Program *prg, Tree **sp, Ref *ref, Tree *v )
{
	treeDownref( prg, sp, ref->kid->tree );
	ref->kid->tree = v;
}

Tree *getRhsEl( Program *prg, Tree *lhs, long position )
{
	Kid *pos = treeChild( prg, lhs );
	while ( position > 0 ) {
		pos = pos->next;
		position -= 1;
	}
	return pos->tree;
}

Tree *colm_get_rhs_val( Program *prg, Tree *tree, int *a )
{
	int i, len = a[0];
	for ( i = 0; i < len; i++ ) {
		int prodNum = a[1 + i * 2];
		int childNum = a[1 + i * 2 + 1];
		if ( tree->prodNum == prodNum )
			return getRhsEl( prg, tree, childNum );
	}
	return 0;
}

void setField( Program *prg, Tree *tree, long field, Tree *value )
{
	assert( tree->refs == 1 );
	if ( value != 0 )
		assert( value->refs >= 1 );
	setAttr( tree, field, value );
}

Tree *getField( Tree *tree, Word field )
{
	return colm_get_attr( tree, field );
}

Kid *getFieldKid( Tree *tree, Word field )
{
	return getAttrKid( tree, field );
}

Tree *getFieldSplit( Program *prg, Tree *tree, Word field )
{
	Tree *val = colm_get_attr( tree, field );
	Tree *split = splitTree( prg, val );
	setAttr( tree, field, split );
	return split;
}

void setUiterCur( Program *prg, UserIter *uiter, Tree *tree )
{
	uiter->ref.kid->tree = tree;
}

void setTriterCur( Program *prg, TreeIter *iter, Tree *tree )
{
	iter->ref.kid->tree = tree;
}

Tree *getPtrVal( Pointer *ptr )
{
	return ptr->value->tree;
}

Tree *getPtrValSplit( Program *prg, Pointer *ptr )
{
	Tree *val = ptr->value->tree;
	Tree *split = splitTree( prg, val );
	ptr->value->tree = split;
	return split;
}

/* This must traverse in the same order that the bindId assignments are done
 * in. */
int matchPattern( Tree **bindings, Program *prg, long pat, Kid *kid, int checkNext )
{
	PatConsNode *nodes = prg->rtd->patReplNodes;

	/* match node, recurse on children. */
	if ( pat != -1 && kid != 0 ) {
		if ( nodes[pat].id == kid->tree->id ) {
			/* If the pattern node has data, then this means we need to match
			 * the data against the token data. */
			if ( nodes[pat].data != 0 ) {
				/* Check the length of token text. */
				if ( nodes[pat].length != stringLength( kid->tree->tokdata ) )
					return false;

				/* Check the token text data. */
				if ( nodes[pat].length > 0 && memcmp( nodes[pat].data, 
						stringData( kid->tree->tokdata ), nodes[pat].length ) != 0 )
					return false;
			}

			/* No failure, all okay. */
			if ( nodes[pat].bindId > 0 ) {
				bindings[nodes[pat].bindId] = kid->tree;
			}

			/* If we didn't match a terminal duplicate of a nonterm then check
			 * down the children. */
			if ( !nodes[pat].stop ) {
				/* Check for failure down child branch. */
				int childCheck = matchPattern( bindings, prg, 
						nodes[pat].child, treeChild( prg, kid->tree ), true );
				if ( ! childCheck )
					return false;
			}

			/* If checking next, then look for failure there. */
			if ( checkNext ) {
				int nextCheck = matchPattern( bindings, prg, 
						nodes[pat].next, kid->next, true );
				if ( ! nextCheck )
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


long cmpTree( Program *prg, const Tree *tree1, const Tree *tree2 )
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
		if ( ((Pointer*)tree1)->value < ((Pointer*)tree2)->value )
			return -1;
		else if ( ((Pointer*)tree1)->value > ((Pointer*)tree2)->value )
			return 1;
	}
	else if ( tree1->id == LEL_ID_INT ) {
		if ( ((Int*)tree1)->value < ((Int*)tree2)->value )
			return -1;
		else if ( ((Int*)tree1)->value > ((Int*)tree2)->value )
			return 1;
	}
	else if ( tree1->id == LEL_ID_STR ) {
		cmpres = cmpString( ((Str*)tree1)->value, ((Str*)tree2)->value );
		if ( cmpres != 0 )
			return cmpres;
	}
	else {
		if ( tree1->tokdata == 0 && tree2->tokdata != 0 )
			return -1;
		else if ( tree1->tokdata != 0 && tree2->tokdata == 0 )
			return 1;
		else if ( tree1->tokdata != 0 && tree2->tokdata != 0 ) {
			cmpres = cmpString( tree1->tokdata, tree2->tokdata );
			if ( cmpres != 0 )
				return cmpres;
		}
	}

	Kid *kid1 = treeChild( prg, tree1 );
	Kid *kid2 = treeChild( prg, tree2 );

	while ( true ) {
		if ( kid1 == 0 && kid2 == 0 )
			return 0;
		else if ( kid1 == 0 && kid2 != 0 )
			return -1;
		else if ( kid1 != 0 && kid2 == 0 )
			return 1;
		else {
			cmpres = cmpTree( prg, kid1->tree, kid2->tree );
			if ( cmpres != 0 )
				return cmpres;
		}
		kid1 = kid1->next;
		kid2 = kid2->next;
	}
}


void splitRef( Program *prg, Tree ***psp, Ref *fromRef )
{
	/* Go up the chain of kids, turing the pointers down. */
	Ref *last = 0, *ref = fromRef, *next = 0;
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
			Ref *nextDown = ref->next;
			while ( nextDown != 0 && nextDown->kid == ref->kid )
				nextDown = nextDown->next;

			Kid *oldNextKidDown = nextDown != 0 ? nextDown->kid : 0;
			Kid *newNextKidDown = 0;

			Tree *newTree = copyTree( prg, ref->kid->tree, 
					oldNextKidDown, &newNextKidDown );
			treeUpref( newTree );
			
			/* Downref the original. Don't need to consider freeing because
			 * refs were > 1. */
			ref->kid->tree->refs -= 1;

			while ( ref != 0 && ref != nextDown ) {
				next = ref->next;
				ref->next = 0;

				ref->kid->tree = newTree;
				ref = next;
			}

			/* Correct kid pointers down from ref. */
			while ( nextDown != 0 && nextDown->kid == oldNextKidDown ) {
				nextDown->kid = newNextKidDown;
				nextDown = nextDown->next;
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

void splitIterCur( Program *prg, Tree ***psp, TreeIter *iter )
{
	if ( iter->ref.kid == 0 )
		return;
	
	splitRef( prg, psp, &iter->ref );
}

Tree *setListMem( List *list, Half field, Tree *value )
{
	assert( list->refs == 1 );
	if ( value != 0 )
		assert( value->refs >= 1 );

	Tree *existing = 0;
	switch ( field ) {
		case 0:
			existing = list->head->value;
			list->head->value = value;
			break;
		case 1:
			existing = list->tail->value;
			list->tail->value = value;
			break;
		default:
			assert( false );
			break;
	}
	return existing;
}

TreePair mapRemove( Program *prg, Map *map, Tree *key )
{
	MapEl *mapEl = mapImplFind( prg, map, key );
	TreePair result = { 0, 0 };
	if ( mapEl != 0 ) {
		mapDetach( prg, map, mapEl );
		result.key = mapEl->key;
		result.val = mapEl->tree;
		mapElFree( prg, mapEl );
	}

	return result;
}

Tree *mapUnstore( Program *prg, Map *map, Tree *key, Tree *existing )
{
	Tree *stored = 0;
	if ( existing == 0 ) {
		MapEl *mapEl = mapDetachByKey( prg, map, key );
		stored = mapEl->tree;
		mapElFree( prg, mapEl );
	}
	else {
		MapEl *mapEl = mapImplFind( prg, map, key );
		stored = mapEl->tree;
		mapEl->tree = existing;
	}
	return stored;
}

Tree *mapFind( Program *prg, Map *map, Tree *key )
{
	MapEl *mapEl = mapImplFind( prg, map, key );
	return mapEl == 0 ? 0 : mapEl->tree;
}

long mapLength( Map *map )
{
	return map->treeSize;
}

void listAppend2( Program *prg, List *list, Tree *val )
{
	assert( list->refs == 1 );
	if ( val != 0 )
		assert( val->refs >= 1 );
	ListEl *listEl = listElAllocate( prg );
	listEl->value = val;
	listAppend( list, listEl );
}

Tree *listRemoveEnd( Program *prg, List *list )
{
	Tree *tree = list->tail->value;
	listElFree( prg, listDetachLast( list ) );
	return tree;
}

Tree *getListMem( List *list, Word field )
{
	Tree *result = 0;
	switch ( field ) {
		case 0: 
			result = list->head->value;
			break;
		case 1: 
			result = list->tail->value;
			break;
		default:
			assert( false );
			break;
	}
	return result;
}

Tree *getParserMem( Parser *parser, Word field )
{
	Tree *result = 0;
	switch ( field ) {
		case 0:
			result = parser->result;
			break;
		case 1:
			result = parser->pdaRun->parseErrorText;
			break;
		default:
			assert( false );
			break;
	}
	return result;
}

Tree *getListMemSplit( Program *prg, List *list, Word field )
{
	Tree *sv = 0;
	switch ( field ) {
		case 0: 
			sv = splitTree( prg, list->head->value );
			list->head->value = sv; 
			break;
		case 1: 
			sv = splitTree( prg, list->tail->value );
			list->tail->value = sv; 
			break;
		default:
			assert( false );
			break;
	}
	return sv;
}


int mapInsert( Program *prg, Map *map, Tree *key, Tree *element )
{
	MapEl *mapEl = mapInsertKey( prg, map, key, 0 );

	if ( mapEl != 0 ) {
		mapEl->tree = element;
		return true;
	}

	return false;
}

void mapUnremove( Program *prg, Map *map, Tree *key, Tree *element )
{
	MapEl *mapEl = mapInsertKey( prg, map, key, 0 );
	assert( mapEl != 0 );
	mapEl->tree = element;
}

Tree *mapUninsert( Program *prg, Map *map, Tree *key )
{
	MapEl *el = mapDetachByKey( prg, map, key );
	Tree *val = el->tree;
	mapElFree( prg, el );
	return val;
}

Tree *mapStore( Program *prg, Map *map, Tree *key, Tree *element )
{
	Tree *oldTree = 0;
	MapEl *elInTree = 0;
	MapEl *mapEl = mapInsertKey( prg, map, key, &elInTree );

	if ( mapEl != 0 )
		mapEl->tree = element;
	else {
		/* Element with key exists. Overwriting the value. */
		oldTree = elInTree->tree;
		elInTree->tree = element;
	}

	return oldTree;
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

static Tree *treeSearchKid( Program *prg, Kid *kid, long id )
{
	/* This node the one? */
	if ( kid->tree->id == id )
		return kid->tree;

	Tree *res = 0;

	/* Search children. */
	Kid *child = treeChild( prg, kid->tree );
	if ( child != 0 )
		res = treeSearchKid( prg, child, id );
	
	/* Search siblings. */
	if ( res == 0 && kid->next != 0 )
		res = treeSearchKid( prg, kid->next, id );

	return res;	
}

Tree *treeSearch( Program *prg, Tree *tree, long id )
{
	Tree *res = 0;
	if ( tree->id == id )
		res = tree;
	else {
		Kid *child = treeChild( prg, tree );
		if ( child != 0 )
			res = treeSearchKid( prg, child, id );
	}
	return res;
}

static Location *locSearchKid( Program *prg, Kid *kid )
{
	/* This node the one? */
	if ( kid->tree->tokdata != 0 && kid->tree->tokdata->location != 0 )
		return kid->tree->tokdata->location;

	Location *res = 0;

	/* Search children. */
	Kid *child = treeChild( prg, kid->tree );
	if ( child != 0 )
		res = locSearchKid( prg, child );
	
	/* Search siblings. */
	if ( res == 0 && kid->next != 0 )
		res = locSearchKid( prg, kid->next );

	return res;	
}

Location *locSearch( Program *prg, Tree *tree )
{
	Location *res = 0;
	if ( tree->tokdata != 0 && tree->tokdata->location != 0 )
		return tree->tokdata->location;

	Kid *child = treeChild( prg, tree );
	if ( child != 0 )
		res = locSearchKid( prg, child );

	return res;
}

struct colm_location *colm_find_location( Program *prg, Tree *tree )
{
	return locSearch( prg, tree );
}

void xmlEscapeData( struct colm_print_args *printArgs, const char *data, long len )
{
	int i;
	for ( i = 0; i < len; i++ ) {
		if ( data[i] == '<' )
			printArgs->out( printArgs, "&lt;", 4 );
		else if ( data[i] == '>' )
			printArgs->out( printArgs, "&gt;", 4 );
		else if ( data[i] == '&' )
			printArgs->out( printArgs, "&amp;", 5 );
		else if ( (32 <= data[i] && data[i] <= 126) || 
				data[i] == '\t' || data[i] == '\n' || data[i] == '\r' )
		{
			printArgs->out( printArgs, &data[i], 1 );
		}
		else {
			char out[64];
			sprintf( out, "&#%u;", ((unsigned)data[i]) );
			printArgs->out( printArgs, out, strlen(out) );
		}
	}
}

void initStrCollect( StrCollect *collect )
{
	collect->data = (char*) malloc( BUFFER_INITIAL_SIZE );
	collect->allocated = BUFFER_INITIAL_SIZE;
	collect->length = 0;
}

void strCollectDestroy( StrCollect *collect )
{
	free( collect->data );
}

void strCollectAppend( StrCollect *collect, const char *data, long len )
{
	long newLen = collect->length + len;
	if ( newLen > collect->allocated ) {
		collect->allocated = newLen * 2;
		collect->data = (char*) realloc( collect->data, collect->allocated );
	}
	memcpy( collect->data + collect->length, data, len );
	collect->length += len;
}
		
void strCollectClear( StrCollect *collect )
{
	collect->length = 0;
}

#define INT_SZ 32

void printStr( struct colm_print_args *printArgs, Head *str )
{
	printArgs->out( printArgs, (char*)(str->data), str->length );
}

void appendCollect( struct colm_print_args *args, const char *data, int length )
{
	strCollectAppend( (StrCollect*) args->arg, data, length );
}

void appendFile( struct colm_print_args *args, const char *data, int length )
{
	fwrite( data, 1, length, (FILE*)args->arg );
}

void appendFd( struct colm_print_args *args, const char *data, int length )
{
	int res = write( (long)args->arg, data, length );
	if ( res != 0 ) {
		message( "write error\n" );
	}
}

Tree *treeTrim( struct colm_program *prg, Tree **sp, Tree *tree )
{
	debug( prg, REALM_PARSE, "attaching left ignore\n" );

	/* Make the ignore list for the left-ignore. */
	Tree *leftIgnore = treeAllocate( prg );
	leftIgnore->id = LEL_ID_IGNORE;
	leftIgnore->flags |= AF_SUPPRESS_RIGHT;

	tree = pushLeftIgnore( prg, tree, leftIgnore );

	debug( prg, REALM_PARSE, "attaching ignore right\n" );

	/* Copy the ignore list first if we need to attach it as a right
	 * ignore. */
	Tree *rightIgnore = 0;
	rightIgnore = treeAllocate( prg );
	rightIgnore->id = LEL_ID_IGNORE;
	rightIgnore->flags |= AF_SUPPRESS_LEFT;

	tree = pushRightIgnore( prg, tree, rightIgnore );

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

void printKid( Program *prg, Tree **sp, struct colm_print_args *printArgs, Kid *kid )
{
	enum ReturnType rt;
	Kid *parent = 0;
	Kid *leadingIgnore = 0;
	enum VisitType visitType;
	int flags = 0;

	/* Iterate the kids passed in. We are expecting a next, which will allow us
	 * to print the trailing ignore list. */
	while ( kid != 0 ) {
		vm_push( (SW) Done );
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
		vm_push( (SW)parent );
		vm_push( (SW)kid );
		parent = kid;
		kid = treeLeftIgnoreKid( prg, kid->tree );
		vm_push( (SW) CollectIgnoreLeft );
		goto rec_call;
		rec_return_ign_left:
		kid = (Kid*)vm_pop();
		parent = (Kid*)vm_pop();
	}

	if ( kid->tree->id == LEL_ID_IGNORE )
		visitType = IgnoreWrapper;
	else if ( parent != 0 && parent->tree->id == LEL_ID_IGNORE )
		visitType = IgnoreData;
	else if ( kid->tree->id < prg->rtd->firstNonTermId )
		visitType = Term;
	else
		visitType = NonTerm;
	
	debug( prg, REALM_PRINT, "visit type: %d\n", visitType );

	if ( visitType == IgnoreData ) {
		debug( prg, REALM_PRINT, "putting %p on ignore list\n", kid->tree );
		Kid *newIgnore = kidAllocate( prg );
		newIgnore->next = leadingIgnore;
		leadingIgnore = newIgnore;
		leadingIgnore->tree = kid->tree;
		goto skip_node;
	}

	if ( visitType == IgnoreWrapper ) {
		Kid *newIgnore = kidAllocate( prg );
		newIgnore->next = leadingIgnore;
		leadingIgnore = newIgnore;
		leadingIgnore->tree = kid->tree;
		/* Don't skip. */
	}

	/* print leading ignore? Triggered by terminals. */
	if ( visitType == Term ) {
		/* Reverse the leading ignore list. */
		if ( leadingIgnore != 0 ) {
			Kid *ignore = 0, *last = 0;

			/* Reverse the list and take the opportunity to implement the
			 * suppress left. */
			while ( true ) {
				Kid *next = leadingIgnore->next;
				leadingIgnore->next = last;

				if ( leadingIgnore->tree->flags & AF_SUPPRESS_LEFT ) {
					/* We are moving left. Chop off the tail. */
					debug( prg, REALM_PRINT, "suppressing left\n" );
					freeKidList( prg, next );
					break;
				}

				if ( next == 0 )
					break;

				last = leadingIgnore;
				leadingIgnore = next;
			}

			/* Print the leading ignore list. Also implement the suppress right
			 * in the process. */
			if ( printArgs->comm && (!printArgs->trim || (flags & TF_TERM_SEEN && kid->tree->id > 0)) ) {	
				ignore = leadingIgnore;
				while ( ignore != 0 ) {
					if ( ignore->tree->flags & AF_SUPPRESS_RIGHT )
						break;

					if ( ignore->tree->id != LEL_ID_IGNORE ) {
						vm_push( (SW)visitType );
						vm_push( (SW)leadingIgnore );
						vm_push( (SW)ignore );
						vm_push( (SW)parent );
						vm_push( (SW)kid );

						leadingIgnore = 0;
						kid = ignore;
						parent = 0;

						debug( prg, REALM_PRINT, "rec call on %p\n", kid->tree );
						vm_push( (SW) RecIgnoreList );
						goto rec_call;
						rec_return_il:

						kid = (Kid*)vm_pop();
						parent = (Kid*)vm_pop();
						ignore = (Kid*)vm_pop();
						leadingIgnore = (Kid*)vm_pop();
						visitType = (enum VisitType)vm_pop();
					}

					ignore = ignore->next;
				}
			}

			/* Free the leading ignore list. */
			freeKidList( prg, leadingIgnore );
			leadingIgnore = 0;
		}
	}

	if ( visitType == Term || visitType == NonTerm ) {
		/* Open the tree. */
		printArgs->open_tree( prg, sp, printArgs, parent, kid );
	}

	if ( visitType == Term )
		flags |= TF_TERM_SEEN;

	if ( visitType == Term || visitType == IgnoreData ) {
		/* Print contents. */
		if ( kid->tree->id < prg->rtd->firstNonTermId ) {
			debug( prg, REALM_PRINT, "printing terminal %p\n", kid->tree );
			if ( kid->tree->id != 0 )
				printArgs->print_term( prg, sp, printArgs, kid );
		}
	}

	/* Print children. */
	Kid *child = printArgs->attr ? 
		treeAttr( prg, kid->tree ) : 
		treeChild( prg, kid->tree );

	if ( child != 0 ) {
		vm_push( (SW)visitType );
		vm_push( (SW)parent );
		vm_push( (SW)kid );
		parent = kid;
		kid = child;
		while ( kid != 0 ) {
			vm_push( (SW) ChildPrint );
			goto rec_call;
			rec_return:
			kid = kid->next;
		}
		kid = (Kid*)vm_pop();
		parent = (Kid*)vm_pop();
		visitType = (enum VisitType)vm_pop();
	}

	if ( visitType == Term || visitType == NonTerm ) {
		/* close the tree. */
		printArgs->close_tree( prg, sp, printArgs, parent, kid );
	}

skip_node:

	/* If not currently skipping ignore data, then print it. Ignore data can
	 * be associated with terminals and nonterminals. */
	if ( kid->tree->flags & AF_RIGHT_IGNORE ) {
		debug( prg, REALM_PRINT, "right ignore\n" );
		vm_push( (SW)parent );
		vm_push( (SW)kid );
		parent = kid;
		kid = treeRightIgnoreKid( prg, kid->tree );
		vm_push( (SW) CollectIgnoreRight );
		goto rec_call;
		rec_return_ign_right:
		kid = (Kid*)vm_pop();
		parent = (Kid*)vm_pop();
	}

/* For skiping over content on null. */
skip_null:

	rt = (enum ReturnType)vm_pop();
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

void colm_print_tree_args( Program *prg, Tree **sp, struct colm_print_args *printArgs, Tree *tree )
{
	if ( tree == 0 )
		printArgs->out( printArgs, "NIL", 3 );
	else {
		/* This term tree allows us to print trailing ignores. */
		Tree termTree;
		memset( &termTree, 0, sizeof(termTree) );

		Kid kid, term;
		term.tree = &termTree;
		term.next = 0;
		term.flags = 0;

		kid.tree = tree;
		kid.next = &term;
		kid.flags = 0;

		printKid( prg, sp, printArgs, &kid );
	}
}

void colm_print_term_tree( Program *prg, Tree **sp, struct colm_print_args *printArgs, Kid *kid )
{
	debug( prg, REALM_PRINT, "printing term %p\n", kid->tree );

	if ( kid->tree->id == LEL_ID_INT ) {
		char buf[INT_SZ];
		sprintf( buf, "%ld", ((Int*)kid->tree)->value );
		printArgs->out( printArgs, buf, strlen(buf) );
	}
	else if ( kid->tree->id == LEL_ID_BOOL ) {
		if ( ((Int*)kid->tree)->value )
			printArgs->out( printArgs, "true", 4 );
		else
			printArgs->out( printArgs, "false", 5 );
	}
	else if ( kid->tree->id == LEL_ID_PTR ) {
		char buf[INT_SZ];
		printArgs->out( printArgs, "#", 1 );
		sprintf( buf, "%p", (void*) ((Pointer*)kid->tree)->value );
		printArgs->out( printArgs, buf, strlen(buf) );
	}
	else if ( kid->tree->id == LEL_ID_STR ) {
		printStr( printArgs, ((Str*)kid->tree)->value );
	}
	else if ( kid->tree->id == LEL_ID_STREAM ) {
		char buf[INT_SZ];
		printArgs->out( printArgs, "#", 1 );
		sprintf( buf, "%p", (void*) ((Stream*)kid->tree)->in->file );
		printArgs->out( printArgs, buf, strlen(buf) );
	}
	else if ( kid->tree->tokdata != 0 && 
			stringLength( kid->tree->tokdata ) > 0 )
	{
		printArgs->out( printArgs, stringData( kid->tree->tokdata ), 
				stringLength( kid->tree->tokdata ) );
	}
}


void colm_print_null( Program *prg, Tree **sp, struct colm_print_args *args, Kid *parent, Kid *kid )
{
}

void openTreeXml( Program *prg, Tree **sp, struct colm_print_args *args, Kid *parent, Kid *kid )
{
	/* Skip the terminal that is for forcing trailing ignores out. */
	if ( kid->tree->id == 0 )
		return;

	LangElInfo *lelInfo = prg->rtd->lelInfo;

	/* List flattening: skip the repeats and lists that are a continuation of
	 * the list. */
	if ( parent != 0 && parent->tree->id == kid->tree->id && kid->next == 0 &&
			( lelInfo[parent->tree->id].repeat || lelInfo[parent->tree->id].list ) )
	{
		return;
	}

	const char *name = lelInfo[kid->tree->id].xmlTag;
	args->out( args, "<", 1 );
	args->out( args, name, strlen( name ) );
	args->out( args, ">", 1 );
}

void printTermXml( Program *prg, Tree **sp, struct colm_print_args *printArgs, Kid *kid )
{
	//Kid *child;

	/*child = */ treeChild( prg, kid->tree );
	if ( kid->tree->id == LEL_ID_PTR ) {
		char ptr[32];
		sprintf( ptr, "%p\n", (void*)((Pointer*)kid->tree)->value );
		printArgs->out( printArgs, ptr, strlen(ptr) );
	}
	else if ( kid->tree->id == LEL_ID_BOOL ) {
		if ( ((Int*)kid->tree)->value )
			printArgs->out( printArgs, "true", 4 );
		else
			printArgs->out( printArgs, "false", 5 );
	}
	else if ( kid->tree->id == LEL_ID_INT ) {
		char ptr[32];
		sprintf( ptr, "%ld", ((Int*)kid->tree)->value );
		printArgs->out( printArgs, ptr, strlen(ptr) );
	}
	else if ( kid->tree->id == LEL_ID_STR ) {
		Head *head = (Head*) ((Str*)kid->tree)->value;

		xmlEscapeData( printArgs, (char*)(head->data), head->length );
	}
	else if ( 0 < kid->tree->id && kid->tree->id < prg->rtd->firstNonTermId &&
			kid->tree->id != LEL_ID_IGNORE &&
			kid->tree->tokdata != 0 && 
			stringLength( kid->tree->tokdata ) > 0 )
	{
		xmlEscapeData( printArgs, stringData( kid->tree->tokdata ), 
				stringLength( kid->tree->tokdata ) );
	}
}


void closeTreeXml( Program *prg, Tree **sp, struct colm_print_args *args, Kid *parent, Kid *kid )
{
	/* Skip the terminal that is for forcing trailing ignores out. */
	if ( kid->tree->id == 0 )
		return;

	LangElInfo *lelInfo = prg->rtd->lelInfo;

	/* List flattening: skip the repeats and lists that are a continuation of
	 * the list. */
	if ( parent != 0 && parent->tree->id == kid->tree->id && kid->next == 0 &&
			( lelInfo[parent->tree->id].repeat || lelInfo[parent->tree->id].list ) )
	{
		return;
	}

	const char *name = lelInfo[kid->tree->id].xmlTag;
	args->out( args, "</", 2 );
	args->out( args, name, strlen( name ) );
	args->out( args, ">", 1 );
}

void printTreeCollect( Program *prg, Tree **sp, StrCollect *collect, Tree *tree, int trim )
{
	struct colm_print_args printArgs = { collect, true, false, trim, &appendCollect, 
			&colm_print_null, &colm_print_term_tree, &colm_print_null };
	colm_print_tree_args( prg, sp, &printArgs, tree );
}

void printTreeFile( Program *prg, Tree **sp, FILE *out, Tree *tree, int trim )
{
	struct colm_print_args printArgs = { out, true, false, trim, &appendFile, 
			&colm_print_null, &colm_print_term_tree, &colm_print_null };
	colm_print_tree_args( prg, sp, &printArgs, tree );
}

void printTreeFd( Program *prg, Tree **sp, int fd, Tree *tree, int trim )
{
	struct colm_print_args printArgs = { (void*)((long)fd), true, false, trim, &appendFd,
			&colm_print_null, &colm_print_term_tree, &colm_print_null };
	colm_print_tree_args( prg, sp, &printArgs, tree );
}

void printXmlStdout( Program *prg, Tree **sp, Tree *tree, int commAttr, int trim )
{
	struct colm_print_args printArgs = { stdout, commAttr, commAttr, trim, &appendFile, 
			&openTreeXml, &printTermXml, &closeTreeXml };
	colm_print_tree_args( prg, sp, &printArgs, tree );
}


