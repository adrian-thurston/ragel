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

#include "bytecode.h"
#include "pdarun.h"
#include "fsmrun.h"
#include "pdarun.h"
#include <iostream>
#include <string.h>
#include <stdlib.h>

using std::cout;
using std::cerr;
using std::endl;
using std::ostream;


void printStr( ostream &out, Head *str )
{
	out.write( (char*)(str->data), str->length );
}


/* Note that this function causes recursion, thought it is not a big
 * deal since the recursion it is only caused by nonterminals that are ignored. */
void printIgnoreList( ostream &out, Tree **sp, Program *prg, Tree *tree )
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
		printTree( out, sp, prg, ignore->tree );
	}
}

void printKid( ostream &out, Tree **&sp, Program *prg, Kid *kid, bool printIgnore )
{
	Tree **root = vm_ptop();
	Kid *child;

rec_call:
	/* If not currently skipping ignore data, then print it. Ignore data can
	 * be associated with terminals and nonterminals. */
	if ( printIgnore && treeIgnore( prg, kid->tree ) != 0 ) {
		/* Ignorelists are reversed. */
		printIgnoreList( out, sp, prg, kid->tree );
		printIgnore = false;
	}

	if ( kid->tree->id < prg->rtd->firstNonTermId ) {
		/* Always turn on ignore printing when we get to a token. */
		printIgnore = true;

		if ( kid->tree->id == LEL_ID_INT )
			out << ((Int*)kid->tree)->value;
		else if ( kid->tree->id == LEL_ID_BOOL ) {
			if ( ((Int*)kid->tree)->value )
				out << "true";
			else
				out << "false";
		}
		else if ( kid->tree->id == LEL_ID_PTR )
			out << '#' << (void*) ((Pointer*)kid->tree)->value;
		else if ( kid->tree->id == LEL_ID_STR )
			printStr( out, ((Str*)kid->tree)->value );
		else if ( kid->tree->id == LEL_ID_STREAM )
			out << '#' << (void*) ((Stream*)kid->tree)->file;
		else if ( kid->tree->tokdata != 0 && 
				stringLength( kid->tree->tokdata ) > 0 )
		{
			out.write( stringData( kid->tree->tokdata ), 
					stringLength( kid->tree->tokdata ) );
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

void printTree( ostream &out, Tree **&sp, Program *prg, Tree *tree )
{
	if ( tree == 0 )
		out << "NIL";
	else {
		Kid kid;
		kid.tree = tree;
		kid.next = 0;
		printKid( out, sp, prg, &kid, false );
	}
}


void xmlEscapeData( const char *data, long len )
{
	for ( int i = 0; i < len; i++ ) {
		if ( data[i] == '<' )
			cout << "&lt;";
		else if ( data[i] == '>' )
			cout << "&gt;";
		else if ( data[i] == '&' )
			cout << "&amp;";
		else if ( 32 <= data[i] && data[i] <= 126 )
			cout << data[i];
		else
			cout << "&#" << ((unsigned)data[i]) << ';';
	}
}

/* Might be a good idea to include this in the printXmlKid function since
 * it is recursive and can eat up stac, however it's probably not a big deal
 * since the additional stack depth is only caused for nonterminals that are
 * ignored. */
void printXmlIgnoreList( Tree **sp, Program *prg, Tree *tree, long depth )
{
	Kid *ignore = treeIgnore( prg, tree );
	while ( ignore != 0 ) {
		printXmlKid( sp, prg, ignore, true, depth );
		ignore = ignore->next;
	}
}

void printXmlKid( Tree **&sp, Program *prg, Kid *kid, bool commAttr, int depth )
{
	Kid *child;
	Tree **root = vm_ptop();
	long i, objectLength;
	LangElInfo *lelInfo = prg->rtd->lelInfo;

	long kidNum = 0;;

rec_call:

	if ( kid->tree == 0 ) {
		for ( i = 0; i < depth; i++ )
			cout << "  ";

		cout << "NIL" << endl;
	}
	else {
		/* First print the ignore tokens. */
		if ( commAttr )
			printXmlIgnoreList( sp, prg, kid->tree, depth );

		for ( i = 0; i < depth; i++ )
			cout << "  ";

		/* Open the tag. Afterwards we will either print data underneath it or
		 * we will close it off immediately. */
		cout << '<' << lelInfo[kid->tree->id].name;

		/* If this is an attribute then give the node an attribute number. */
		if ( vm_ptop() != root ) {
			objectLength = lelInfo[((Kid*)vm_top())->tree->id].objectLength;
			if ( kidNum < objectLength )
				cout << " an=\"" << kidNum << '"';
		}

		objectLength = lelInfo[kid->tree->id].objectLength;
		child = treeChild( prg, kid->tree );
		if ( (commAttr && objectLength > 0) || child != 0 ) {
			cout << '>' << endl;
			vm_push( (SW) kidNum );
			vm_push( (SW) kid );

			kidNum = 0;
			kid = kid->tree->child;

			/* Skip over attributes if not printing comments and attributes. */
			if ( ! commAttr )
				kid = child;

			while ( kid != 0 ) {
				/* FIXME: I don't think we need this check for ignore any more. */
				if ( kid->tree == 0 || !lelInfo[kid->tree->id].ignore ) {
					depth++;
					goto rec_call;
					rec_return:
					depth--;
				}
				
				kid = kid->next;
				kidNum += 1;

				/* If the parent kid is a repeat then skip this node and go
				 * right to the first child (repeated item). */
				if ( lelInfo[((Kid*)vm_top())->tree->id].repeat )
					kid = kid->tree->child;

				/* If we have a kid and the parent is a list (recursive prod of
				 * list) then go right to the first child. */
				if ( kid != 0 && lelInfo[((Kid*)vm_top())->tree->id].list )
					kid = kid->tree->child;
			}

			kid = (Kid*) vm_pop();
			kidNum = (long) vm_pop();

			for ( i = 0; i < depth; i++ )
				cout << "  ";
			cout << "</" << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_PTR ) {
			cout << '>' << (void*)((Pointer*)kid->tree)->value << 
					"</" << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_BOOL ) {
			if ( ((Int*)kid->tree)->value )
				cout << ">true</";
			else
				cout << ">false</";
			cout << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_INT ) {
			cout << '>' << ((Int*)kid->tree)->value << 
					"</" << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_STR ) {
			Head *head = (Head*) ((Str*)kid->tree)->value;

			cout << '>';
			xmlEscapeData( (char*)(head->data), head->length );
			cout << "</" << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( 0 < kid->tree->id && kid->tree->id < prg->rtd->firstNonTermId &&
				kid->tree->tokdata != 0 && 
				stringLength( kid->tree->tokdata ) > 0 && 
				!lelInfo[kid->tree->id].literal )
		{
			cout << '>';
			xmlEscapeData( stringData( kid->tree->tokdata ), 
					stringLength( kid->tree->tokdata ) );
			cout << "</" << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else
			cout << "/>" << endl;
	}

	if ( vm_ptop() != root )
		goto rec_return;
}

void printXmlTree( Tree **&sp, Program *prg, Tree *tree, bool commAttr )
{
	Kid kid;
	kid.tree = tree;
	kid.next = 0;
	printXmlKid( sp, prg, &kid, commAttr, 0 );
}

void iterFind( Program *prg, Tree **&sp, TreeIter *iter, int tryFirst )
{
	int anyTree = iter->searchId == prg->rtd->anyId;
	Tree **top = iter->stackRoot;
	Kid *child;

rec_call:
	if ( tryFirst && ( iter->ref.kid->tree->id == iter->searchId || anyTree ) )
		return;
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
}

Tree *treeIterAdvance( Program *prg, Tree **&sp, TreeIter *iter )
{
	assert( iter->stackSize == iter->stackRoot - vm_ptop() );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the root. */
		iter->ref = iter->rootRef;
		iterFind( prg, sp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iterFind( prg, sp, iter, false );
	}

	iter->stackSize = iter->stackRoot - vm_ptop();

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

Tree *treeIterNextChild( Program *prg, Tree **&sp, TreeIter *iter )
{
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

	return ( iter->ref.kid ? prg->trueVal : prg->falseVal );
}

Tree *treeRevIterPrevChild( Program *prg, Tree **&sp, RevTreeIter *iter )
{
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

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

void iterFindRepeat( Program *prg, Tree **&sp, TreeIter *iter, int tryFirst )
{
	int anyTree = iter->searchId == prg->rtd->anyId;
	Tree **top = iter->stackRoot;
	Kid *child;

rec_call:
	if ( tryFirst && ( iter->ref.kid->tree->id == iter->searchId || anyTree ) )
		return;
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
}

Tree *treeIterNextRepeat( Program *prg, Tree **&sp, TreeIter *iter )
{
	assert( iter->stackSize == iter->stackRoot - vm_ptop() );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the root. */
		iter->ref = iter->rootRef;
		iterFindRepeat( prg, sp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iterFindRepeat( prg, sp, iter, false );
	}

	iter->stackSize = iter->stackRoot - vm_ptop();

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

void iter_find_rev_repeat( Program *prg, Tree **&sp, TreeIter *iter, int tryFirst )
{
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
		if ( iter->ref.kid->tree->id == iter->searchId || anyTree )
			return;
	}

	return;
}

Tree *treeIterPrevRepeat( Program *prg, Tree **&sp, TreeIter *iter )
{
	assert( iter->stackSize == iter->stackRoot - vm_ptop() );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the root. */
		iter->ref = iter->rootRef;
		iter_find_rev_repeat( prg, sp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iter_find_rev_repeat( prg, sp, iter, false );
	}

	iter->stackSize = iter->stackRoot - vm_ptop();

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

void listAppend( Program *prg, List *list, Tree *val )
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

void splitRef( Tree **&sp, Program *prg, Ref *fromRef )
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

void splitIterCur( Tree **&sp, Program *prg, TreeIter *iter )
{
	if ( iter->ref.kid == 0 )
		return;
	
	splitRef( sp, prg, &iter->ref );
}

extern "C" long cmpTree( Program *prg, const Tree *tree1, const Tree *tree2 )
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

