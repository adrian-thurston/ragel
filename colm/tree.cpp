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
#include "dlistval.h"
#include "fsmrun.h"
#include "pdarun.h"

#define push(i) (*(--sp) = (i))
#define pop() (*sp++)

void stream_free( Program *prg, Stream *s )
{
	delete s->scanner;
	delete s->in;
	if ( s->file != 0 )
		fclose( s->file );
	prg->mapElPool.free( (MapEl*)s );
}

/* We can't make recursive calls here since the tree we are freeing may be
 * very large. Need the VM stack. */
void tree_free( Program *prg, Tree **sp, Tree *tree )
{
	Tree **top = sp;

free_tree:
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	long genericId = lelInfo[tree->id].genericId;
	if ( genericId > 0 ) {
		GenericInfo *generic = &prg->rtd->genericInfo[genericId];
		if ( generic->type == GEN_LIST ) {
			List *list = (List*) tree;
			ListEl *el = list->head;
			while ( el != 0 ) {
				ListEl *next = el->next;
				push( el->value );
				prg->listElPool.free( el );
				el = next;
			}
			prg->mapElPool.free( (MapEl*)list );
		}
		else if ( generic->type == GEN_MAP ) {
			Map *map = (Map*)tree;
			MapEl *el = map->head;
			while ( el != 0 ) {
				MapEl *next = el->next;
				push( el->key );
				push( el->tree );
				prg->mapElPool.free( el );
				el = next;
			}
			prg->mapElPool.free( (MapEl*)map );
		}
		else
			assert(false);
	}
	else {
		if ( tree->id == LEL_ID_STR ) {
			Str *str = (Str*) tree;
			string_free( prg, str->value );
			prg->treePool.free( tree );
		}
		else if ( tree->id == LEL_ID_BOOL || tree->id == LEL_ID_INT )
			prg->treePool.free( tree );
		else if ( tree->id == LEL_ID_PTR ) {
			//Pointer *ptr = (Pointer*)tree;
			//push( ptr->value->tree );
			//prg->kidPool.free( ptr->value );
			prg->treePool.free( tree );
		}
		else if ( tree->id == LEL_ID_STREAM )
			stream_free( prg, (Stream*) tree );
		else { 
			if ( tree->alg != 0 ) {
				//assert( ! (tree->alg->flags & AF_HAS_RCODE) );
				push( tree->alg->parsed );
				prg->algPool.free( tree->alg );
			}
			string_free( prg, tree->tokdata );

			Kid *child = tree->child;
			while ( child != 0 ) {
				Kid *next = child->next;
				push( child->tree );
				prg->kidPool.free( child );
				child = next;
			}

			prg->treePool.free( tree );
		}
	}

	/* Any trees to downref? */
	while ( sp != top ) {
		tree = pop();
		if ( tree != 0 ) {
			assert( tree->refs > 0 );
			tree->refs -= 1;
			if ( tree->refs == 0 )
				goto free_tree;
		}
	}
}

void tree_upref( Tree *tree )
{
	if ( tree != 0 )
		tree->refs += 1;
}

void tree_downref( Program *prg, Tree **sp, Tree *tree )
{
	if ( tree != 0 ) {
		assert( tree->refs > 0 );
		tree->refs -= 1;
		if ( tree->refs == 0 )
			tree_free( prg, sp, tree );
	}
}

/* Find the first child of a tree. */
Kid *tree_child( Program *prg, Tree *tree )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Kid *kid = tree->child;

	/* Skip over attributes. */
	long objectLength = lelInfo[tree->id].objectLength;
	for ( long a = 0; a < objectLength; a++ )
		kid = kid->next;

	/* Skip over ignore tokens. */
	while ( kid != 0 && lelInfo[kid->tree->id].ignore )
		kid = kid->next;
	return kid;
}

/* Find the first child of a tree. */
Kid *tree_extract_child( Program *prg, Tree *tree )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Kid *kid = tree->child, *last = 0;

	/* Skip over attributes. */
	long objectLength = lelInfo[tree->id].objectLength;
	for ( long a = 0; a < objectLength; a++ ) {
		last = kid;
		kid = kid->next;
	}

	/* Skip over ignore tokens. */
	while ( kid != 0 && lelInfo[kid->tree->id].ignore ) {
		last = kid;
		kid = kid->next;
	}

	if ( last == 0 )
		tree->child = 0;
	else
		last->next = 0;

	return kid;
}


Kid *tree_ignore( Program *prg, Tree *tree )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Kid *ignore = tree->child;

	/* Skip over attributes. */
	long objectLength = lelInfo[tree->id].objectLength;
	for ( long a = 0; a < objectLength; a++ )
		ignore = ignore->next;

	/* Check for ignore tokens, there may not be any. */
	if ( ignore != 0 && !lelInfo[ignore->tree->id].ignore )
		ignore = 0;
	return ignore;
}

bool tree_is_ignore( Program *prg, Kid *kid )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	return kid != 0 && lelInfo[kid->tree->id].ignore;
}

