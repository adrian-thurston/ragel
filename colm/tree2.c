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
#include "map.h"
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

/* New tree has zero ref. */
Tree *copyRealTree( Program *prg, Tree *tree, Kid *oldNextDown, 
		Kid **newNextDown, int parseTree )
{
	/* Need to keep a lookout for next down. If 
	 * copying it, return the copy. */
	Tree *newTree;
	if ( parseTree ) {
		newTree = (Tree*) parseTreeAllocate( prg );
		newTree->flags |= AF_PARSE_TREE;
	}
	else {
		newTree = treeAllocate( prg );
	}

	newTree->id = tree->id;
	newTree->tokdata = stringCopy( prg, tree->tokdata );

	/* Copy the child list. Start with ignores, then the list. */
	Kid *child = tree->child, *last = 0;

	/* Left ignores. */
	if ( tree->flags & AF_LEFT_IGNORE ) {
		newTree->flags |= AF_LEFT_IGNORE;
		Kid *newHeader = copyIgnoreList( prg, child );

		/* Always the head. */
		newTree->child = newHeader;

		child = child->next;
		last = newHeader;
	}

	/* Right ignores. */
	if ( tree->flags & AF_RIGHT_IGNORE ) {
		newTree->flags |= AF_RIGHT_IGNORE;
		Kid *newHeader = copyIgnoreList( prg, child );
		if ( last == 0 )
			newTree->child = newHeader;
		else
			last->next = newHeader;
		child = child->next;
		last = newHeader;
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
	#ifdef COLM_LOG_BYTECODE
	if ( colm_log_bytecode ) {
		cerr << "splitting list: " << list << " refs: " << 
				list->refs << endl;
	}
	#endif

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
	#ifdef COLM_LOG_BYTECODE
	if ( colm_log_bytecode ) {
		cerr << "splitting map: " << map << " refs: " << 
				map->refs << endl;
	}
	#endif

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
		tree = copyRealTree( prg, tree, oldNextDown, newNextDown, false );
	}

	assert( tree->refs == 0 );
	return tree;
}

Tree *splitTree( Program *prg, Tree *tree )
{
	if ( tree != 0 ) {
		assert( tree->refs >= 1 );

		if ( tree->refs > 1 ) {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "splitting tree: " << tree << " refs: " << 
						tree->refs << endl;
			}
			#endif

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
			Accum *accum = (Accum*)mapElAllocate( prg );
			accum->id = genericInfo->langElId;
			accum->genericInfo = genericInfo;
			accum->fsmRun = malloc( sizeof(FsmRun) );
			accum->pdaRun = malloc( sizeof(PdaRun) );

			/* Start off the parsing process. */
			initPdaRun( accum->pdaRun, prg, prg->rtd->pdaTables, 
					accum->fsmRun, genericInfo->parserId, false, false, 0 );
			initFsmRun( accum->fsmRun, prg );
			newToken( accum->pdaRun, accum->fsmRun );

			newGeneric = (Tree*) accum;
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
	Tree **top = sp;
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
			Accum *accum = (Accum*)tree;
			free( accum->fsmRun );
			cleanParser( sp, accum->pdaRun );
			clearContext( accum->pdaRun, sp );
			rcodeDownrefAll( prg, sp, accum->pdaRun->allReverseCode );
			free( accum->pdaRun );
			treeDownref( prg, sp, (Tree*)accum->stream );
			mapElFree( prg, (MapEl*)accum );
		}
		else
			assert(false);
	}
	else {
		if ( tree->id == LEL_ID_STR ) {
			Str *str = (Str*) tree;
			stringFree( prg, str->value );
			treeFree( prg, tree );
		}
		else if ( tree->id == LEL_ID_BOOL || tree->id == LEL_ID_INT )
			treeFree( prg, tree );
		else if ( tree->id == LEL_ID_PTR ) {
			//Pointer *ptr = (Pointer*)tree;
			//vm_push( ptr->value->tree );
			//kidFree( prg, ptr->value );
			treeFree( prg, tree );
		}
		else if ( tree->id == LEL_ID_STREAM )
			streamFree( prg, (Stream*) tree );
		else { 
			stringFree( prg, tree->tokdata );
			Kid *child = tree->child;

			/* Left ignore trees. */
			if ( tree->flags & AF_LEFT_IGNORE ) {
				Kid *ic = (Kid*)child->tree;
				while ( ic != 0 ) {
					Kid *next = ic->next;
					vm_push( ic->tree );
					kidFree( prg, ic );
					ic = next;
				}
			
				Kid *next = child->next;
				kidFree( prg, child );
				child = next;
			}

			/* Right ignore trees. */
			if ( tree->flags & AF_RIGHT_IGNORE ) {
				Kid *ic = (Kid*)child->tree;
				while ( ic != 0 ) {
					Kid *next = ic->next;
					vm_push( ic->tree );
					kidFree( prg, ic );
					ic = next;
				}

				Kid *next = child->next;
				kidFree( prg, child );
				child = next;
			}

			/* Attributes and grammar-based children. */
			while ( child != 0 ) {
				Kid *next = child->next;
				vm_push( child->tree );
				kidFree( prg, child );
				child = next;
			}

			if ( tree->flags & AF_PARSE_TREE )
				parseTreeFree( prg, (ParseTree*)tree );
			else
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

/* Find the first child of a tree. */
Kid *treeExtractChild( Program *prg, Tree *tree )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Kid *kid = tree->child, *last = 0;

	if ( tree->flags & AF_LEFT_IGNORE )
		kid = kid->next;
	if ( tree->flags & AF_RIGHT_IGNORE )
		kid = kid->next;

	/* Skip over attributes. */
	long objectLength = lelInfo[tree->id].objectLength;
	long a;
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


Kid *treeIgnore( Program *prg, Tree *tree )
{
	if ( tree->flags & AF_LEFT_IGNORE )
		return (Kid*)tree->child->tree;
	return 0;
}

Tree *treeIterDerefCur( TreeIter *iter )
{
	return iter->ref.kid == 0 ? 0 : iter->ref.kid->tree;
}

void refSetValue( Ref *ref, Tree *v )
{
	Kid *firstKid = ref->kid;
	while ( ref != 0 && ref->kid == firstKid ) {
		ref->kid->tree = v;
		ref = ref->next;
	}
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

void setField( Program *prg, Tree *tree, long field, Tree *value )
{
	assert( tree->refs == 1 );
	if ( value != 0 )
		assert( value->refs >= 1 );
	setAttr( tree, field, value );
}

Tree *getField( Tree *tree, Word field )
{
	return getAttr( tree, field );
}

Kid *getFieldKid( Tree *tree, Word field )
{
	return getAttrKid( tree, field );
}

Tree *getFieldSplit( Program *prg, Tree *tree, Word field )
{
	Tree *val = getAttr( tree, field );
	Tree *split = splitTree( prg, val );
	setAttr( tree, field, split );
	return split;
}

void setTriterCur( TreeIter *iter, Tree *tree )
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
	PatReplNode *nodes = prg->rtd->patReplNodes;

	#ifdef COLM_LOG_MATCH
	if ( colm_log_match ) {
		LangElInfo *lelInfo = prg->rtd->lelInfo;
		cerr << "match pattern " << ( pat == -1 ? "NULL" : lelInfo[nodes[pat].id].name ) <<
				" vs " << ( kid == 0 ? "NULL" : lelInfo[kid->tree->id].name ) << endl;
	}
	#endif

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
				#ifdef COLM_LOG_MATCH
				if ( colm_log_match ) {
					cerr << "bindId: " << nodes[pat].bindId << endl;
				}
				#endif
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


void splitRef( Tree ***psp, Program *prg, Ref *fromRef )
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
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "splitting tree: " << ref->kid << " refs: " << 
						ref->kid->tree->refs << endl;
			}
			#endif

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

void splitIterCur( Tree ***psp, Program *prg, TreeIter *iter )
{
	if ( iter->ref.kid == 0 )
		return;
	
	splitRef( psp, prg, &iter->ref );
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
	TreePair result;
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
	assert( iter->stackSize == iter->stackRoot - *psp );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the root. */
		iter->ref = iter->rootRef;
		iterFind( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iterFind( prg, psp, iter, false );
	}

	iter->stackSize = iter->stackRoot - *psp;

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

Tree *treeIterNextChild( Program *prg, Tree ***psp, TreeIter *iter )
{
	Tree **sp = *psp;
	assert( iter->stackSize == iter->stackRoot - vm_ptop() );
	Kid *kid = 0;

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the first child. */
		Kid *child = treeChild( prg, iter->rootRef.kid->tree );

		if ( child == 0 )
			iter->ref.next = 0;
		else {
			/* Make a reference to the root. */
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
	iter->stackSize = iter->stackRoot - vm_ptop();
	*psp = sp;
	return ( iter->ref.kid ? prg->trueVal : prg->falseVal );
}

Tree *treeRevIterPrevChild( Program *prg, Tree ***psp, RevTreeIter *iter )
{
	Tree **sp = *psp;

	assert( iter->stackSize == iter->stackRoot - vm_ptop() );

	if ( iter->kidAtYield != iter->ref.kid ) {
		/* Need to reload the kids. */
		Kid *kid = treeChild( prg, iter->rootRef.kid->tree );
		Kid **dst = (Kid**)iter->stackRoot - 1;
		while ( kid != 0 ) {
			*dst-- = kid;
			kid = kid->next;
		}
	}

	if ( iter->ref.kid == 0 )
		iter->cur = (Kid**)iter->stackRoot - iter->children;
	else
		iter->cur += 1;

	if ( iter->searchId != prg->rtd->anyId ) {
		/* Have a previous item, go to the next sibling. */
		while ( iter->cur != (Kid**)iter->stackRoot && (*iter->cur)->tree->id != iter->searchId )
			iter->cur += 1;
	}

	if ( iter->cur == (Kid**)iter->stackRoot ) {
		iter->ref.next = 0;
		iter->ref.kid = 0;
	}
	else {
		iter->ref.next = &iter->rootRef;
		iter->ref.kid = *iter->cur;
	}

	/* We will use this to detect a split above the iterated tree. */
	iter->kidAtYield = iter->ref.kid;

	iter->stackSize = iter->stackRoot - vm_ptop();

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
	assert( iter->stackSize == iter->stackRoot - *psp );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the root. */
		iter->ref = iter->rootRef;
		iterFindRepeat( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iterFindRepeat( prg, psp, iter, false );
	}

	iter->stackSize = iter->stackRoot - *psp;

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

void iter_find_rev_repeat( Program *prg, Tree ***psp, TreeIter *iter, int tryFirst )
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
			iter->ref.kid = ref->kid->tree->child;
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
	assert( iter->stackSize == iter->stackRoot - *psp );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the root. */
		iter->ref = iter->rootRef;
		iter_find_rev_repeat( prg, psp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iter_find_rev_repeat( prg, psp, iter, false );
	}

	iter->stackSize = iter->stackRoot - *psp;

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

Tree *treeSearch( Program *prg, Kid *kid, long id )
{
	/* This node the one? */
	if ( kid->tree->id == id )
		return kid->tree;

	Tree *res = 0;

	/* Search children. */
	Kid *child = treeChild( prg, kid->tree );
	if ( child != 0 )
		res = treeSearch( prg, child, id );
	
	/* Search siblings. */
	if ( res == 0 && kid->next != 0 )
		res = treeSearch( prg, kid->next, id );

	return res;	
}

Tree *treeSearch2( Program *prg, Tree *tree, long id )
{
	Tree *res = 0;
	if ( tree->id == id )
		res = tree;
	else {
		Kid *child = treeChild( prg, tree );
		if ( child != 0 )
			res = treeSearch( prg, child, id );
	}
	return res;
}


