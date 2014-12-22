/*
 *  Copyright 2001, 2002, 2006, 2011 Adrian Thurston <thurston@complang.org>
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

void FsmAp::expandConds( StateAp *fromState, TransAp *trans, const CondSet &fromCS, const CondSet &mergedCS )
{
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
				addInTrans( cond, cti );

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

void FsmAp::expandCondTransitions( StateAp *fromState,
		TransAp *destTrans, TransAp *srcTrans )
{
	CondSet destCS, srcCS;
	CondSet mergedCS;

	if ( destTrans->condSpace != 0 )
		destCS.insert( destTrans->condSpace->condSet );

	if ( srcTrans->condSpace != 0 )
		srcCS.insert( srcTrans->condSpace->condSet );
	
	mergedCS.insert( destCS );
	mergedCS.insert( srcCS );

	expandConds( fromState, destTrans, destCS, mergedCS );
	expandConds( fromState, srcTrans, srcCS, mergedCS );

	CondSpace *mergedCondSpace = addCondSpace( mergedCS );
	destTrans->condSpace = mergedCondSpace;
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


void FsmAp::embedCondition( MergeData &md, StateAp *state, Action *condAction, bool sense )
{
	for ( TransList::Iter tr = state->outList; tr.lte(); tr++ ) {

		/* The original cond set. */
		CondSet origCS;
		if ( tr->condSpace != 0 )
			origCS.insert( tr->condSpace->condSet );

		/* The merged cond set. */
		CondSet mergedCS;
		mergedCS.insert( origCS );
		mergedCS.insert( condAction );

		/* Allocate a cond space for the merged spet. */
		CondSpace *mergedCondSpace = addCondSpace( mergedCS );
		tr->condSpace = mergedCondSpace;

		/* Translate original condition values, making space for the new bit
		 * (possibly) introduced by the condition embedding. */
		for ( CondList::Iter cti = tr->tcap()->condList; cti.lte(); cti++ ) {
			long origVal = cti->key.getVal();
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

			if ( origVal != newVal ) {
				std::cerr << "orig: " << origVal << " new: " << newVal << std::endl;
				cti->key = newVal;
			}

			/* Now set the new bit appropriately. Since it defaults to zero we
			 * only take action if sense is positive. */
			if ( sense ) {
				Action **cim = mergedCS.find( condAction );
				int pos = cim - mergedCS.data;
				cti->key = cti->key.getVal() | (1 << pos);
			}
		}
	}
}

void FsmAp::embedCondition( StateAp *state, Action *condAction, bool sense )
{
	MergeData md;

	/* Turn on misfit accounting to possibly catch the old start state. */
	setMisfitAccounting( true );

	/* Worker. */
	embedCondition( md, state, condAction, sense );

	/* Fill in any states that were newed up as combinations of others. */
	fillInStates( md );

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );
}
