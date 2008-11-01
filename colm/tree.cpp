/*
 *  Copyright 2008 Adrian Thurston <thurston@cs.queensu.ca>
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

void tree_free( Program *prg, Tree *tree )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	long genericId = lelInfo[tree->id].genericId;
	if ( genericId > 0 ) {
		GenericInfo *generic = &prg->rtd->genericInfo[genericId];
		if ( generic->type == GEN_LIST )
			list_free( prg, (List*)tree );
		else if ( generic->type == GEN_MAP )
			map_free( prg, (Map*)tree );
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
			//tree_downref( prg, ptr->value->tree );
			//prg->kidPool.free( ptr->value );
			prg->treePool.free( tree );
		}
		else if ( tree->id == LEL_ID_STREAM )
			stream_free( prg, (Stream*) tree );
		else { 
			if ( tree->alg != 0 ) {
				//assert( ! (tree->alg->flags & AF_HAS_RCODE) );
				tree_downref( prg, tree->alg->parsed );
				prg->algPool.free( tree->alg );
			}
			string_free( prg, tree->tokdata );

			Kid *child = tree->child;
			while ( child != 0 ) {
				Kid *next = child->next;
				tree_downref( prg, child->tree );
				prg->kidPool.free( child );
				child = next;
			}

			prg->treePool.free( tree );
		}
	}
}

void tree_upref( Tree *tree )
{
	if ( tree != 0 )
		tree->refs += 1;
};

void tree_downref( Program *prg, Tree *tree )
{
	if ( tree != 0 ) {
		assert( tree->refs > 0 );
		tree->refs -= 1;
		if ( tree->refs == 0 )
			tree_free( prg, tree );
	}
};

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

