/*
 *  Copyright 2008 Adrian Thurston <thurston@complang.org>
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

#include "pdarun.h"
#include "tree.h"
#include "pool.h"
#include "bytecode2.h"
#include "debug.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define true 1
#define false 0

void initUserIter( UserIter *userIter, Tree **stackRoot, long argSize, long searchId )
{
	userIter->stackRoot = stackRoot;
	userIter->argSize = argSize;
	userIter->stackSize = 0;
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

Tree *getAttr( Tree *tree, long pos )
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


Stream *openStreamFile( Program *prg, FILE *file )
{
	Stream *res = (Stream*)mapElAllocate( prg );
	res->id = LEL_ID_STREAM;
	res->file = file;
	res->in = newInputStreamFile( file );
	initInputStream( res->in );
	return res;
}

Stream *openStreamFd( Program *prg, long fd )
{
	Stream *res = (Stream*)mapElAllocate( prg );
	res->id = LEL_ID_STREAM;
	res->in = newInputStreamFd( fd );
	initInputStream( res->in );
	return res;
}

Stream *openFile( Program *prg, Tree *name, Tree *mode )
{
	Head *headName = ((Str*)name)->value;
	Head *headMode = ((Str*)mode)->value;

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
	free(fileName);
	return openStreamFile( prg, file );
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

Kid *constructReplacementKid( Tree **bindings, Program *prg, Kid *prev, long pat );

Kid *constructIgnoreList( Program *prg, long pat )
{
	PatReplNode *nodes = prg->rtd->patReplNodes;
	long ignore = nodes[pat].ignore;

	Kid *first = 0, *last = 0;
	while ( ignore >= 0 ) {
		Head *ignoreData = stringAllocPointer( prg, nodes[ignore].data, nodes[ignore].length );

		Tree *ignTree = treeAllocate( prg );
		ignTree->refs = 1;
		ignTree->id = nodes[ignore].id;
		ignTree->tokdata = ignoreData;

		Kid *ignKid = kidAllocate( prg );
		ignKid->tree = ignTree;
		ignKid->next = 0;

		if ( last == 0 )
			first = ignKid;
		else
			last->next = ignKid;

		ignore = nodes[ignore].next;
		last = ignKid;
	}

	return first;
}

/* Returns an uprefed tree. Saves us having to downref and bindings to zero to
 * return a zero-ref tree. */
Tree *constructReplacementTree( Tree **bindings, Program *prg, long pat )
{
	PatReplNode *nodes = prg->rtd->patReplNodes;
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Tree *tree = 0;

	if ( nodes[pat].bindId > 0 ) {
		/* All bindings have been uprefed. */
		tree = bindings[nodes[pat].bindId];

		long ignore = nodes[pat].ignore;
		if ( ignore >= 0 ) {
			Kid *ignore = constructIgnoreList( prg, pat );

			tree = splitTree( prg, tree );

			Kid *ignoreHead = kidAllocate( prg );
			ignoreHead->next = tree->child;
			tree->child = ignoreHead;

			ignoreHead->tree = (Tree*) ignore;
			tree->flags |= AF_LEFT_IGNORE;
		}
	}
	else {
		tree = treeAllocate( prg );
		tree->id = nodes[pat].id;
		tree->refs = 1;
		tree->tokdata = nodes[pat].length == 0 ? 0 :
				stringAllocPointer( prg, 
				nodes[pat].data, nodes[pat].length );

		int objectLength = lelInfo[tree->id].objectLength;

		Kid *attrs = allocAttrs( prg, objectLength );
		Kid *ignore = constructIgnoreList( prg, pat );
		Kid *child = constructReplacementKid( bindings, prg, 
				0, nodes[pat].child );

		tree->child = kidListConcat( attrs, child );
		if ( ignore != 0 ) {
			Kid *ignoreHead = kidAllocate( prg );
			ignoreHead->next = tree->child;
			tree->child = ignoreHead;

			ignoreHead->tree = (Tree*) ignore;
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
	PatReplNode *nodes = prg->rtd->patReplNodes;
	Kid *kid = 0;

	if ( pat != -1 ) {
		kid = kidAllocate( prg );
		kid->tree = constructReplacementTree( bindings, prg, pat );

		/* Recurse down next. */
		Kid *next = constructReplacementKid( bindings, prg, 
				kid, nodes[pat].next );

		kid->next = next;
	}

	return kid;
}

Tree *makeToken2( Tree **root, Program *prg, long nargs )
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
			treeUpref( getAttr( tree, id) );
		}
	}
	return tree;
}

Tree *makeTree( Tree **root, Program *prg, long nargs )
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

void printStr2( FILE *out, Head *str )
{
	fwrite( (char*)(str->data), str->length, 1, out );
}

/* Note that this function causes recursion, thought it is not a big
 * deal since the recursion it is only caused by nonterminals that are ignored. */
void printIgnoreList2( FILE *out, Tree **sp, Program *prg, Tree *tree )
{
	Kid *ignore = treeIgnore( prg, tree );

	/* Record the root of the stack and push everything. */
	Tree **root = vm_ptop();
	while ( ignore != 0 ) {
		vm_push( (SW)ignore );
		ignore = ignore->next;
	}

	/* Pop them off and print. */
	while ( vm_ptop() != root ) {
		ignore = (Kid*) vm_pop();
		printTree2( out, sp, prg, ignore->tree );
	}
}


void printKid2( FILE *out, Tree **sp, Program *prg, Kid *kid, int printIgnore )
{
	Tree **root = vm_ptop();
	Kid *child;

rec_call:
	/* If not currently skipping ignore data, then print it. Ignore data can
	 * be associated with terminals and nonterminals. */
	if ( printIgnore && treeIgnore( prg, kid->tree ) != 0 ) {
		/* Ignorelists are reversed. */
		printIgnoreList2( out, sp, prg, kid->tree );
		printIgnore = false;
	}

	if ( kid->tree->id < prg->rtd->firstNonTermId ) {
		/* Always turn on ignore printing when we get to a token. */
		printIgnore = true;

		if ( kid->tree->id == LEL_ID_INT )
			fprintf( out, "%ld", ((Int*)kid->tree)->value );
		else if ( kid->tree->id == LEL_ID_BOOL ) {
			if ( ((Int*)kid->tree)->value )
				fprintf( out, "true" );
			else
				fprintf( out, "false" );
		}
		else if ( kid->tree->id == LEL_ID_PTR )
			fprintf( out, "#%p", (void*) ((Pointer*)kid->tree)->value );
		else if ( kid->tree->id == LEL_ID_STR )
			printStr2( out, ((Str*)kid->tree)->value );
		else if ( kid->tree->id == LEL_ID_STREAM )
			fprintf( out, "#%p", ((Stream*)kid->tree)->file );
		else if ( kid->tree->tokdata != 0 && 
				stringLength( kid->tree->tokdata ) > 0 )
		{
			fwrite( stringData( kid->tree->tokdata ), 
					stringLength( kid->tree->tokdata ), 1, out );
		}
	}
	else {
		/* Non-terminal. */
		child = treeChild( prg, kid->tree );
		if ( child != 0 ) {
			vm_push( (SW)kid );
			kid = child;
			while ( kid != 0 ) {
				goto rec_call;
				rec_return:
				kid = kid->next;
			}
			kid = (Kid*)vm_pop();
		}
	}

	if ( vm_ptop() != root )
		goto rec_return;
}

void printTree2( FILE *out, Tree **sp, Program *prg, Tree *tree )
{
	if ( tree == 0 )
		fprintf( out, "NIL" );
	else {
		Kid kid;
		kid.tree = tree;
		kid.next = 0;
		printKid2( out, sp, prg, &kid, false );
	}
}

void streamFree( Program *prg, Stream *s )
{
	free( s->in );
	if ( s->file != 0 )
		fclose( s->file );
	mapElFree( prg, (MapEl*)s );
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
