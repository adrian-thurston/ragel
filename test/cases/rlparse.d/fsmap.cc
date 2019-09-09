/*
 *  Copyright 2002-2004 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "fsmgraph.h"
#include <iostream>
using std::endl;

/* Insert an action into an action table. */
void ActionTable::setAction( int ordering, Action *action )
{
	/* Multi-insert in case specific instances of an action appear in a
	 * transition more than once. */
	insertMulti( ordering, action );
}

/* Set all the action from another action table in this table. */
void ActionTable::setActions( const ActionTable &other )
{
	for ( ActionTable::Iter action = other; action.lte(); action++ )
		insertMulti( action->key, action->value );
}

void ActionTable::setActions( int *orderings, Action **actions, int nActs )
{
	for ( int a = 0; a < nActs; a++ )
		insertMulti( orderings[a], actions[a] );
}

bool ActionTable::hasAction( Action *action )
{
	for ( int a = 0; a < length(); a++ ) {
		if ( data[a].value == action )
			return true;
	}
	return false;
}

/* Insert an action into an action table. */
void LmActionTable::setAction( int ordering, LongestMatchPart *action )
{
	/* Multi-insert in case specific instances of an action appear in a
	 * transition more than once. */
	insertMulti( ordering, action );
}

/* Set all the action from another action table in this table. */
void LmActionTable::setActions( const LmActionTable &other )
{
	for ( LmActionTable::Iter action = other; action.lte(); action++ )
		insertMulti( action->key, action->value );
}

void ErrActionTable::setAction( int ordering, Action *action, int transferPoint )
{
	insertMulti( ErrActionTableEl( action, ordering, transferPoint ) );
}

void ErrActionTable::setActions( const ErrActionTable &other )
{
	for ( ErrActionTable::Iter act = other; act.lte(); act++ )
		insertMulti( ErrActionTableEl( act->action, act->ordering, act->transferPoint ) );
}

/* Insert a priority into this priority table. Looks out for priorities on
 * duplicate keys. */
void PriorTable::setPrior( int ordering, PriorDesc *desc )
{
	PriorEl *lastHit = 0;
	PriorEl *insed = insert( PriorEl(ordering, desc), &lastHit );
	if ( insed == 0 ) {
		/* This already has a priority on the same key as desc. Overwrite the
		 * priority if the ordering is larger (later in time). */
		if ( ordering >= lastHit->ordering )
			*lastHit = PriorEl( ordering, desc );
	}
}

/* Set all the priorities from a priorTable in this table. */
void PriorTable::setPriors( const PriorTable &other )
{
	/* Loop src priorities once to overwrite duplicates. */
	PriorTable::Iter priorIt = other;
	for ( ; priorIt.lte(); priorIt++ )
		setPrior( priorIt->ordering, priorIt->desc );
}

