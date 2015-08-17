/*
 *  Copyright 2001, 2002, 2006, 2011 Adrian Thurston <thurston@complang.org>
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

/*
 * Setting conditions and merging states with conditions are similar activities
 * when expressed in code. The critical difference is that a merge is a union
 * of multiple paths. We have to take both paths. Setting a condition, however,
 * is a restriction. We have to expand the transition to follow both values of
 * the condition, then remove the one that is not set.
 */

#include "fsmgraph.h"
#include "mergesort.h"
#include "parsedata.h"

#include <assert.h>
#include <iostream>

long TransAp::condFullSize() 
	{ return condSpace == 0 ? 1 : condSpace->fullSize(); }

void FsmAp::expandCondKeys( CondKeySet &condKeys, CondSpace *fromSpace,
		CondSpace *mergedSpace )
{
	CondSet fromCS, mergedCS;

	if ( fromSpace != 0 )
		fromCS.insert( fromSpace->condSet );

	if ( mergedSpace != 0 )
		mergedCS.insert( mergedSpace->condSet );
	
	/* Need to transform condition element to the merged set. */
	for ( int cti = 0; cti < condKeys.length(); cti++ ) {
		long origVal = condKeys[cti];
		long newVal = 0;

		/* Iterate the bit positions in the from set. */
		for ( CondSet::Iter csi = fromCS; csi.lte(); csi++ ) {
			/* If set, find it in the merged set and flip the bit to 1. */
			if ( origVal & (1 << csi.pos()) ) {
				/* The condition is set. Find the bit position in the merged
				 * set. */
				Action **cim = mergedCS.find( *csi );
				long bitPos = (cim - mergedCS.data);
				newVal |= 1 << bitPos;
			}
		}

		if ( origVal != newVal )
			condKeys[cti] = newVal;
	}

	/* Need to double up the whole transition list for each condition test in
	 * merged that is not in from. The one we add has the bit in question set.
	 * */
	for ( CondSet::Iter csi = mergedCS; csi.lte(); csi++ ) {
		Action **cim = fromCS.find( *csi );
		if ( cim == 0 ) {
			CondKeySet newItems;
			newItems.append( condKeys );
			for ( int cti = 0; cti < condKeys.length(); cti++ ) {
				int key = condKeys[cti] | (1 << csi.pos());
				newItems.insert( key );
			}

			condKeys.setAs( newItems );
		}
	}
}

void FsmAp::expandConds( StateAp *fromState, TransAp *trans,
		CondSpace *fromSpace, CondSpace *mergedSpace )
{
	CondSet fromCS, mergedCS;

	if ( fromSpace != 0 )
		fromCS.insert( fromSpace->condSet );

	if ( mergedSpace != 0 )
		mergedCS.insert( mergedSpace->condSet );
	
	/* Need to transform condition element to the merged set. */
	for ( CondList::Iter cti = trans->tcap()->condList; cti.lte(); cti++ ) {
		long origVal = cti->key.getVal();
		long newVal = 0;

		/* Iterate the bit positions in the from set. */
		for ( CondSet::Iter csi = fromCS; csi.lte(); csi++ ) {
			/* If set, find it in the merged set and flip the bit to 1. */
			if ( origVal & (1 << csi.pos()) ) {
				/* The condition is set. Find the bit position in the merged
				 * set. */
				Action **cim = mergedCS.find( *csi );
				long bitPos = (cim - mergedCS.data);
				newVal |= 1 << bitPos;
			}
		}

		if ( origVal != newVal )
			cti->key = newVal;
	}

	/* Need to double up the whole transition list for each condition test in
	 * merged that is not in from. The one we add has the bit in question set.
	 * */
	for ( CondSet::Iter csi = mergedCS; csi.lte(); csi++ ) {
		Action **cim = fromCS.find( *csi );
		if ( cim == 0 ) {
			CondList newItems;
			for ( CondList::Iter cti = trans->tcap()->condList; cti.lte(); cti++ ) {
				/* Sub-transition for conditions. */
				CondAp *cond = new CondAp( trans );

				/* Attach only if our caller wants the expanded transitions
				 * attached. */
				attachTrans( fromState, cti->toState, cond );
				
				/* Call the user callback to add in the original source transition. */
				addInTrans( cond, cti.ptr );

				cond->key = cti->key.getVal() | (1 << csi.pos());

				newItems.append( cond );
			}

			/* Merge newItems in. Both the condList and newItems are sorted. Make
			 * a sorted list out of them. */
			CondAp *dest = trans->tcap()->condList.head;
			while ( dest != 0 && newItems.head != 0 ) { 
				if ( newItems.head->key.getVal() > dest->key.getVal() ) {
					dest = dest->next;
				}
				else {
					/* Pop the item for insertion. */
					CondAp *ins = newItems.detachFirst();
					trans->tcap()->condList.addBefore( dest, ins );
				}
			}

			/* Append the rest of the items. */
			trans->tcap()->condList.append( newItems );
		}
	}
}

CondSpace *FsmAp::expandCondSpace( TransAp *destTrans, TransAp *srcTrans )
{
	CondSet destCS, srcCS;
	CondSet mergedCS;

	if ( destTrans->condSpace != 0 )
		destCS.insert( destTrans->condSpace->condSet );

	if ( srcTrans->condSpace != 0 )
		srcCS.insert( srcTrans->condSpace->condSet );
	
	mergedCS.insert( destCS );
	mergedCS.insert( srcCS );

	return addCondSpace( mergedCS );
}

StateAp *FsmAp::copyStateForExpansion( StateAp *srcState )
{
	StateAp *newState = new StateAp();
	newState->outCondSpace = srcState->outCondSpace;
	newState->outCondKeys = srcState->outCondKeys;
	return newState;
}

void FsmAp::mergeOutConds( StateAp *destState, StateAp *srcState, bool leaving )
{
	if ( destState == srcState )
		return;

	bool bothFinal = destState->isFinState() && srcState->isFinState();
	bool unionOp = !leaving;
				   
	CondSet destCS, srcCS;
	CondSet mergedCS;

	if ( destState->outCondSpace != 0 )
		destCS.insert( destState->outCondSpace->condSet );

	if ( srcState->outCondSpace != 0 )
		srcCS.insert( srcState->outCondSpace->condSet );
	
	mergedCS.insert( destCS );
	mergedCS.insert( srcCS );

	if ( mergedCS.length() > 0 ) {
		CondSpace *mergedSpace = addCondSpace( mergedCS );

		CondSpace *srcSpace = srcState->outCondSpace;
		CondKeySet srcKeys = srcState->outCondKeys;

		if ( srcSpace != mergedSpace ) {
			/* Prep the key list with zero item if necessary. */
			if ( srcSpace == 0 )
				srcKeys.append( 0 );

			expandCondKeys( srcKeys, srcSpace, mergedSpace );
		}

		if ( destState->outCondSpace != mergedSpace ) {
			/* Prep the key list with zero item if necessary. */
			if ( destState->outCondSpace == 0 )
				destState->outCondKeys.append( 0 );

			/* Now expand the dest. */
			expandCondKeys( destState->outCondKeys, destState->outCondSpace, mergedSpace );
		}

		destState->outCondSpace = mergedSpace;

		if ( unionOp && bothFinal ) {
			/* Keys can come from either. */
			for ( CondKeySet::Iter c = srcKeys; c.lte(); c++ )
				destState->outCondKeys.insert( *c );
		}
		else {
			/* Keys need to be in both sets. */
			for ( long c = 0; c < destState->outCondKeys.length(); ) {
				if ( !srcKeys.find( destState->outCondKeys[c] ) )
					destState->outCondKeys.CondKeyVect::remove( c, 1 );
				else
					c++;
			}
		}
	}
}

CondSpace *FsmAp::addCondSpace( const CondSet &condSet )
{
	CondSpace *condSpace = ctx->condData->condSpaceMap.find( condSet );
	if ( condSpace == 0 ) {
		condSpace = new CondSpace( condSet );
		ctx->condData->condSpaceMap.insert( condSpace );
	}
	return condSpace;
}

TransCondAp *FsmAp::convertToCondAp( StateAp *from, TransDataAp *trans )
{
	TransCondAp *newTrans = new TransCondAp();
	newTrans->lowKey = trans->lowKey;
	newTrans->highKey = trans->highKey;
	newTrans->condSpace = trans->condSpace;

	CondAp *newCond = new CondAp( newTrans );
	newCond->key = 0;
	newTrans->condList.append( newCond );

	newCond->lmActionTable.setActions( trans->lmActionTable );
	newCond->actionTable.setActions( trans->actionTable );
	newCond->priorTable.setPriors( trans->priorTable );

	attachTrans( from, trans->toState, newCond );

	/* Detach in list. */
	detachTrans( from, trans->toState, trans );
	delete trans;

	return newTrans;
}

void FsmAp::convertToCondAp( StateAp *state )
{
	/* First replace TransDataAp with cond versions. */
	TransList destList;
	for ( TransList::Iter tr = state->outList; tr.lte(); ) {
		TransList::Iter next = tr.next();
		if ( tr->plain() ) {
			TransCondAp *newTrans = convertToCondAp( state, tr->tdap() );
			destList.append( newTrans );
		}
		else {
			destList.append( tr );
		}

		tr = next;
	}

	state->outList.abandon();
	state->outList.transfer( destList );
}

void FsmAp::doEmbedCondition( StateAp *state,
		const CondSet &set, const CondKeySet &vals )
{
	convertToCondAp( state );
	
	for ( TransList::Iter tr = state->outList; tr.lte(); tr++ ) {

		/* The source (being embedded). */
		CondSpace *srcSpace = addCondSpace( set );
		CondKeySet srcVals = vals;

		/* Extract cond key set from the condition list. We will use this to
		 * compute the intersection of the cond keys. */
		CondSpace *trSpace = tr->condSpace;
		CondKeySet trVals;
		if ( tr->condSpace == 0 )
			trVals.append( 0 );
		else {
			for ( CondList::Iter cti = tr->tcap()->condList; cti.lte(); cti++ ) {
				long key = cti->key.getVal();
				trVals.append( key );
			}
		}

		/* Construct merged. */
		CondSet mergedCS;
		if ( tr->condSpace != 0 )
			mergedCS.insert( tr->condSpace->condSet );
		mergedCS.insert( set );

		CondSpace *mergedSpace = addCondSpace( mergedCS );

		if ( srcSpace != mergedSpace ) {
			/* Prep the key list with zero item if necessary. */
			if ( srcSpace == 0 )
				srcVals.append( 0 );

			expandCondKeys( srcVals, srcSpace, mergedSpace );
		}

		if ( trSpace != mergedSpace ) {
			/* Don't need to prep the key list with zero item, will be there
			 * (see above). */
			expandCondKeys( trVals, trSpace, mergedSpace );
		}

		/* Implement AND, in two parts. */
		CondKeySet newItems;
		for ( CondKeySet::Iter c = srcVals; c.lte(); c++ ) {
			if ( trVals.find( *c ) )
				newItems.insert( *c );
		}

		for ( CondKeySet::Iter c = trVals; c.lte(); c++ ) {
			if ( srcVals.find( *c ) )
				newItems.insert( *c );
		}

		/* Expand the transitions, then we remove anything not in the computed
		 * list of keys. This approach allows us to embed combinations of
		 * senses, rather than cond-sense pairs. Necessary for out conditions. */
		CondSpace *orig = tr->condSpace;
		tr->condSpace = mergedSpace;
		expandConds( state, tr, orig, mergedSpace );

		/* After expansion, remove anything not in newItems. */
		for ( CondList::Iter cti = tr->tcap()->condList; cti.lte(); ) {
			long key = cti->key.getVal();

			if ( !newItems.find( key ) ) {
				/* Delete. */
				CondList::Iter next = cti.next();

				CondAp *cond = cti;
				detachTrans( state, cond->toState, cond );
				tr->tcap()->condList.detach( cond );
				delete cond;

				cti = next;
			}
			else {
				/* Leave alone. */
				cti++;
			}
		}

	}
}

void FsmAp::embedCondition( StateAp *state, const CondSet &set, const CondKeySet &vals )
{
	/* Turn on misfit accounting to possibly catch the old start state. */
	setMisfitAccounting( true );

	/* Worker. */
	doEmbedCondition( state, set, vals );

	/* Fill in any states that were newed up as combinations of others. */
	fillInStates();

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );
}

void FsmAp::addOutCondition( StateAp *state, Action *condAction, bool sense )
{
	CondSet origCS;
	if ( state->outCondSpace != 0 )
		origCS.insert( state->outCondSpace->condSet );

	CondSet mergedCS;
	mergedCS.insert( origCS );

	bool added = mergedCS.insert( condAction );
	if ( !added ) {

		/* Already exists in the cond set. For every transition, if the
		 * sense is identical to what we are embedding, leave it alone. If
		 * the sense is opposite, delete it. */

		/* Find the position. */
		long pos = 0;
		for ( CondSet::Iter csi = mergedCS; csi.lte(); csi++ ) {
			if ( *csi == condAction )
				pos = csi.pos();
		}

		for ( int cti = 0; cti < state->outCondKeys.length(); ) {
			long key = state->outCondKeys[cti];

			bool set = ( key & ( 1 << pos ) ) != 0;
			if ( sense xor set ) {
				/* Delete. */
				state->outCondKeys.CondKeyVect::remove( cti, 1 );
			}
			else {
				/* Leave alone. */
				cti++;
			}
		}
	}
	else {
		/* Does not exist in the cond set. We will add it. */

		if ( state->outCondSpace == 0 ) {
			/* Note that unlike transitions, we start here with an empty key
			 * list. Add the item */
			state->outCondKeys.append( 0 );
		}

		/* Allocate a cond space for the merged set. */
		CondSpace *mergedCondSpace = addCondSpace( mergedCS );
		state->outCondSpace = mergedCondSpace;

		/* FIXME: assumes one item always. */

		/* Translate original condition values, making space for the new bit
		 * (possibly) introduced by the condition embedding. */
		for ( int cti = 0; cti < state->outCondKeys.length(); cti++ ) {
			long origVal = state->outCondKeys[cti];
			long newVal = 0;

			/* For every set bit in the orig, find it's position in the merged
			 * and set the bit appropriately. */
			for ( CondSet::Iter csi = origCS; csi.lte(); csi++ ) {
				/* If set, find it in the merged set and flip the bit to 1. If
				 * not set, there is nothing to do (convenient eh?) */
				if ( origVal & (1 << csi.pos()) ) {
					/* The condition is set. Find the bit position in the
					 * merged set. */
					Action **cim = mergedCS.find( *csi );
					long bitPos = (cim - mergedCS.data);
					newVal |= 1 << bitPos;
				}
			}

			//std::cerr << "orig: " << origVal << " new: " << newVal << std::endl;

			if ( origVal != newVal )
				state->outCondKeys[cti] = newVal;

			/* Now set the new bit appropriately. Since it defaults to zero we
			 * only take action if sense is positive. */
			if ( sense ) {
				Action **cim = mergedCS.find( condAction );
				int pos = cim - mergedCS.data;
				state->outCondKeys[cti] = state->outCondKeys[cti] | (1 << pos);
			}
		}
	}
}
