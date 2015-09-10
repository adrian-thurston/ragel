/*
 *  Copyright 2001-2015 Adrian Thurston <thurston@complang.org>
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

#include <string.h>
#include <assert.h>
#include "fsmgraph.h"

#include <iostream>
using namespace std;

void FsmAp::attachStateDict( StateAp *from, StateAp *to )
{
	if ( to->stateDictIn == 0 )
		to->stateDictIn = new StateSet;

	bool inserted = to->stateDictIn->insert( from );
	assert( inserted );

	if ( from != to ) {
		if ( misfitAccounting ) {
			if ( to->foreignInTrans == 0 )
				stateList.append( misfitList.detach( to ) );
		}

		to->foreignInTrans += 1;
	}
}

void FsmAp::detachStateDict( StateAp *from, StateAp *to )
{
	bool removed = to->stateDictIn->remove( from );
	assert( removed );

	to->foreignInTrans -= 1;

	if ( from != to ) {
		if ( misfitAccounting ) {
			if ( to->foreignInTrans == 0 )
				misfitList.append( stateList.detach( to ) );
		}
	}
}

void FsmAp::attachToNfa( StateAp *from, StateAp *to, NfaTrans *nfaTrans )
{
	if ( to->nfaIn == 0 )
		to->nfaIn = new NfaInList;

	nfaTrans->fromState = from;
	nfaTrans->toState = to;

	attachToInList( from, to, to->nfaIn->head, nfaTrans );
}

void FsmAp::detachFromNfa( StateAp *from, StateAp *to, NfaTrans *nfaTrans )
{
	nfaTrans->fromState = 0;
	nfaTrans->toState = 0;

	detachFromInList( from, to, to->nfaIn->head, nfaTrans );
}

template< class Head > void FsmAp::attachToInList( StateAp *from,
		StateAp *to, Head *&head, Head *trans )
{
	trans->ilnext = head;
	trans->ilprev = 0;

	/* If in trans list is not empty, set the head->prev to trans. */
	if ( head != 0 )
		head->ilprev = trans;

	/* Now insert ourselves at the front of the list. */
	head = trans;

	/* Keep track of foreign transitions for from and to. */
	if ( from != to ) {
		if ( misfitAccounting ) {
			/* If the number of foreign in transitions is about to go up to 1 then
			 * move it from the misfit list to the main list. */
			if ( to->foreignInTrans == 0 )
				stateList.append( misfitList.detach( to ) );
		}
		
		to->foreignInTrans += 1;
	}
};

/* Detach a transition from an inlist. The head of the inlist must be supplied. */
template< class Head > void FsmAp::detachFromInList( StateAp *from, StateAp *to, 
		Head *&head, Head *trans )
{
	if ( trans->ilprev == 0 ) 
		head = trans->ilnext; 
	else
		trans->ilprev->ilnext = trans->ilnext; 

	if ( trans->ilnext != 0 )
		trans->ilnext->ilprev = trans->ilprev; 
	
	/* Keep track of foreign transitions for from and to. */
	if ( from != to ) {
		to->foreignInTrans -= 1;
		
		if ( misfitAccounting ) {
			/* If the number of foreign in transitions goes down to 0 then move it
			 * from the main list to the misfit list. */
			if ( to->foreignInTrans == 0 )
				misfitList.append( stateList.detach( to ) );
		}
	}
}

CondAp *FsmAp::attachNewCond( TransAp *trans, StateAp *from, StateAp *to, CondKey onChar )
{
	/* Sub-transition for conditions. */
	CondAp *condAp = new CondAp( trans );
	condAp->key = onChar;
	trans->tcap()->condList.append( condAp );

	condAp->fromState = from;
	condAp->toState = to;

	/* Attach in list. */
	if ( to != 0 )
		attachToInList( from, to, to->inCond.head, condAp );

	return condAp;
}

TransAp *FsmAp::attachNewTrans( StateAp *from, StateAp *to, Key lowKey, Key highKey )
{
	/* Make the new transition. */
	TransDataAp *retVal = new TransDataAp();

	/* Make the entry in the out list for the transitions. */
	from->outList.append( retVal );

	/* Set the the keys of the new trans. */
	retVal->lowKey = lowKey;
	retVal->highKey = highKey;

	retVal->fromState = from;
	retVal->toState = to;

	/* Attach in list. */
	if ( to != 0 )
		attachToInList( from, to, to->inTrans.head, retVal );

	return retVal;
}

/* Attach for range lists or for the default transition.  This attach should
 * be used when a transition already is allocated and must be attached to a
 * target state.  Does not handle adding the transition into the out list. */
void FsmAp::attachTrans( StateAp *from, StateAp *to, TransDataAp *trans )
{
	assert( trans->fromState == 0 && trans->toState == 0 );

	trans->fromState = from;
	trans->toState = to;

	if ( to != 0 ) {
		/* For now always attache the one and only condList element. */
		attachToInList( from, to, to->inTrans.head, trans );
	}
}

void FsmAp::attachTrans( StateAp *from, StateAp *to, CondAp *trans )
{
	assert( trans->fromState == 0 && trans->toState == 0 );

	trans->fromState = from;
	trans->toState = to;

	if ( to != 0 ) {
		/* For now always attache the one and only condList element. */
		attachToInList( from, to, to->inCond.head, trans );
	}
}

/* Redirect a transition away from error and towards some state. This is just
 * like attachTrans except it requires fromState to be set and does not touch
 * it. */
void FsmAp::redirectErrorTrans( StateAp *from, StateAp *to, TransDataAp *trans )
{
	assert( trans->fromState != 0 && trans->toState == 0 );
	trans->toState = to;

	if ( to != 0 ) {
		/* Attach using the inList pointer as the head pointer. */
		attachToInList( from, to, to->inTrans.head, trans );
	}
}

void FsmAp::redirectErrorTrans( StateAp *from, StateAp *to, CondAp *trans )
{
	assert( trans->fromState != 0 && trans->toState == 0 );
	trans->toState = to;

	if ( to != 0 ) {
		/* Attach using the inList pointer as the head pointer. */
		attachToInList( from, to, to->inCond.head, trans );
	}
}

/* Detach for out/in lists or for default transition. */
void FsmAp::detachTrans( StateAp *from, StateAp *to, TransDataAp *trans )
{
	assert( trans->fromState == from && trans->toState == to );

	trans->fromState = 0;
	trans->toState = 0;

	if ( to != 0 ) {
		detachFromInList( from, to, to->inTrans.head, trans );
	}
}

void FsmAp::detachTrans( StateAp *from, StateAp *to, CondAp *trans )
{
	assert( trans->fromState == from && trans->toState == to );

	trans->fromState = 0;
	trans->toState = 0;

	if ( to != 0 ) {
		detachFromInList( from, to, to->inCond.head, trans );
	}
}


/* Detach a state from the graph. Detaches and deletes transitions in and out
 * of the state. Empties inList and outList. Removes the state from the final
 * state set. A detached state becomes useless and should be deleted. */
void FsmAp::detachState( StateAp *state )
{
	while ( state->inTrans.head != 0 ) {
		/* Get pointers to the trans and the state. */
		TransDataAp *trans = state->inTrans.head;

		StateAp *fromState = trans->fromState;

		/* Detach the transitions from the source state. */
		detachTrans( fromState, state, trans );
		fromState->outList.detach( trans );
		delete trans;
	}

	/* Detach the in transitions from the inList list of transitions. */
	while ( state->inCond.head != 0 ) {
		/* Get pointers to the trans and the state. */
		CondAp *condAp = state->inCond.head;
		TransAp *trans = condAp->transAp;

		StateAp *fromState = condAp->fromState;

		/* Detach the transitions from the source state. */
		detachTrans( fromState, state, condAp );

		trans->tcap()->condList.detach( condAp );
		delete condAp;

		if ( trans->tcap()->condList.length() == 0 ) {
			/* Ok to delete the transition. */
			fromState->outList.detach( trans );
			delete trans;
		}
	}

	/* Remove the entry points in on the machine. */
	while ( state->entryIds.length() > 0 )
		unsetEntry( state->entryIds[0], state );

	/* Detach out range transitions. */
	for ( TransList::Iter trans = state->outList; trans.lte(); ) {
		TransList::Iter next = trans.next();
		if ( trans->plain() ) {
			detachTrans( state, trans->tdap()->toState, trans->tdap() );
		}
		else {
			for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); ) {
				CondList::Iter next = cond.next();
				detachTrans( state, cond->toState, cond );
				delete cond;
				cond = next;
			}
		}
		delete trans;
		trans = next;
	}

	/* Delete all of the out range pointers. */
	state->outList.abandon();

	/* Unset final stateness before detaching from graph. */
	if ( state->stateBits & STB_ISFINAL )
		finStateSet.remove( state );
	
	if ( state->nfaIn != 0 ) {
		while ( state->nfaIn->head != 0 ) {
			NfaTrans *trans = state->nfaIn->head;
			StateAp *fromState = trans->fromState;

			detachFromNfa( fromState, state, trans );
			fromState->nfaOut->detach( trans );
		}
		delete state->nfaIn;
		state->nfaIn = 0;
	}

	if ( state->nfaOut != 0 ) {
		for ( NfaTransList::Iter t = *state->nfaOut; t.lte(); t++ ) {
			detachFromNfa( t->fromState, t->toState, t );
			state->nfaOut->detach( t );
		}
		delete state->nfaOut;
		state->nfaOut = 0;
	}

	if ( state->stateDictIn != 0 ) {
		for ( StateSet::Iter s = *state->stateDictIn; s.lte(); s++ ) {
			bool removed = (*s)->stateDictEl->stateSet.remove( state );
			assert( removed );
		}

		delete state->stateDictIn;
		state->stateDictIn = 0;
	}

	if ( state->stateDictEl != 0 ) {
		for ( StateSet::Iter s = state->stateDictEl->stateSet; s.lte(); s++ )
			detachStateDict( state, *s );

		stateDict.detach( state->stateDictEl );
		delete state->stateDictEl;
		state->stateDictEl = 0;

		nfaList.detach( state );
	}
}

TransDataAp *FsmAp::dupTransData( StateAp *from, TransDataAp *srcTrans )
{
	/* Make a new transition. */
	TransDataAp *newTrans = new TransDataAp();
	newTrans->condSpace = srcTrans->condSpace;

	attachTrans( from, srcTrans->tdap()->toState, newTrans );
	addInTrans( newTrans, srcTrans->tdap() );

	return newTrans;
}


/* Duplicate a transition. Makes a new transition that is attached to the same
 * dest as srcTrans. The new transition has functions and priority taken from
 * srcTrans. Used for merging a transition in to a free spot. The trans can
 * just be dropped in. It does not conflict with an existing trans and need
 * not be crossed. Returns the new transition. */
TransAp *FsmAp::dupTrans( StateAp *from, TransAp *srcTrans )
{
	if ( srcTrans->plain() ) {
		/* Make a new transition. */
		TransDataAp *newTrans = new TransDataAp();
		newTrans->condSpace = srcTrans->condSpace;

		attachTrans( from, srcTrans->tdap()->toState, newTrans );
		addInTrans( newTrans, srcTrans->tdap() );

		return newTrans;
	}
	else {
		/* Make a new transition. */
		TransAp *newTrans = new TransCondAp();
		newTrans->condSpace = srcTrans->condSpace;

		for ( CondList::Iter sc = srcTrans->tcap()->condList; sc.lte(); sc++ ) {
			/* Sub-transition for conditions. */
			CondAp *newCond = new CondAp( newTrans );
			newCond->key = sc->key;
			newTrans->tcap()->condList.append( newCond );

			/* We can attach the transition, one does not exist. */
			attachTrans( from, sc->toState, newCond );
				
			/* Call the user callback to add in the original source transition. */
			addInTrans( newCond, sc.ptr );
		}

		return newTrans;
	}
}

/* Duplicate a transition. Makes a new transition that is attached to the same
 * dest as srcTrans. The new transition has functions and priority taken from
 * srcTrans. Used for merging a transition in to a free spot. The trans can
 * just be dropped in. It does not conflict with an existing trans and need
 * not be crossed. Returns the new transition. */
CondAp *FsmAp::dupCondTrans( StateAp *from, TransAp *destParent, CondAp *srcTrans )
{
	/* Sub-transition for conditions. */
	CondAp *newCond = new CondAp( destParent );

	/* We can attach the transition, one does not exist. */
	attachTrans( from, srcTrans->toState, newCond );
			
	/* Call the user callback to add in the original source transition. */
	addInTrans( newCond, srcTrans );

	return newCond;
}

/* In crossing, src trans and dest trans both go to existing states. Make one
 * state from the sets of states that src and dest trans go to. */
template< class Trans > Trans *FsmAp::fsmAttachStates( StateAp *from,
			Trans *destTrans, Trans *srcTrans )
{
	/* The priorities are equal. We must merge the transitions. Does the
	 * existing trans go to the state we are to attach to? ie, are we to
	 * simply double up the transition? */
	StateAp *toState = srcTrans->toState;
	StateAp *existingState = destTrans->toState;

	if ( existingState == toState ) {
		/* The transition is a double up to the same state.  Copy the src
		 * trans into itself. We don't need to merge in the from out trans
		 * data, that was done already. */
		addInTrans( destTrans, srcTrans );
	}
	else {
		/* The trans is not a double up. Dest trans cannot be the same as src
		 * trans. Set up the state set. */
		StateSet stateSet;

		/* We go to all the states the existing trans goes to, plus... */
		if ( existingState->stateDictEl == 0 )
			stateSet.insert( existingState );
		else
			stateSet.insert( existingState->stateDictEl->stateSet );

		/* ... all the states that we have been told to go to. */
		if ( toState->stateDictEl == 0 )
			stateSet.insert( toState );
		else
			stateSet.insert( toState->stateDictEl->stateSet );

		/* Look for the state. If it is not there already, make it. */
		StateDictEl *lastFound;
		if ( stateDict.insert( stateSet, &lastFound ) ) {
			/* Make a new state representing the combination of states in
			 * stateSet. It gets added to the fill list.  This means that we
			 * need to fill in it's transitions sometime in the future.  We
			 * don't do that now (ie, do not recurse). */
			StateAp *combinState = addState();

			/* Link up the dict element and the state. */
			lastFound->targState = combinState;
			combinState->stateDictEl = lastFound;

			/* Setup the in links. */
			for ( StateSet::Iter s = stateSet; s.lte(); s++ )
				attachStateDict( combinState, *s );
			
			/* Add to the fill list. */
			nfaList.append( combinState );
		}

		/* Get the state insertted/deleted. */
		StateAp *targ = lastFound->targState;

		/* Detach the state from existing state. */
		detachTrans( from, existingState, destTrans );

		/* Re-attach to the new target. */
		attachTrans( from, targ, destTrans );

		/* Add in src trans to the existing transition that we redirected to
		 * the new state. We don't need to merge in the from out trans data,
		 * that was done already. */
		addInTrans( destTrans, srcTrans );
	}

	return destTrans;
}

/* Two transitions are to be crossed, handle the possibility of either going
 * to the error state. */
template < class Trans > Trans *FsmAp::mergeTrans( StateAp *from,
			Trans *destTrans, Trans *srcTrans )
{
	Trans *retTrans = 0;
	if ( destTrans->toState == 0 && srcTrans->toState == 0 ) {
		/* Error added into error. */
		addInTrans( destTrans, srcTrans );
		retTrans = destTrans;
	}
	else if ( destTrans->toState == 0 && srcTrans->toState != 0 ) {
		/* Non error added into error we need to detach and reattach, */
		detachTrans( from, destTrans->toState, destTrans );
		attachTrans( from, srcTrans->toState, destTrans );
		addInTrans( destTrans, srcTrans );
		retTrans = destTrans;
	}
	else if ( srcTrans->toState == 0 ) {
		/* Dest goes somewhere but src doesn't, just add it it in. */
		addInTrans( destTrans, srcTrans );
		retTrans = destTrans;
	}
	else {
		/* Both go somewhere, run the actual cross. */
		retTrans = fsmAttachStates( from, destTrans, srcTrans );
	}

	return retTrans;
}

/* Find the trans with the higher priority. If src is lower priority then dest then
 * src is ignored. If src is higher priority than dest, then src overwrites dest. If
 * the priorities are equal, then they are merged. */
CondAp *FsmAp::crossCondTransitions( StateAp *from, TransAp *destParent,
		CondAp *destTrans, CondAp *srcTrans )
{
	CondAp *retTrans;

	/* Compare the priority of the dest and src transitions. */
	int compareRes = comparePrior( destTrans->priorTable, srcTrans->priorTable );
	if ( compareRes < 0 ) {
		/* Src trans has a higher priority than dest, src overwrites dest.
		 * Detach dest and return a copy of src. */
		detachTrans( from, destTrans->toState, destTrans );
		retTrans = dupCondTrans( from, destParent, srcTrans );
	}
	else if ( compareRes > 0 ) {
		/* The dest trans has a higher priority, use dest. */
		retTrans = destTrans;
	}
	else {
		/* Src trans and dest trans have the same priority, they must be merged. */
		retTrans = mergeTrans( from, destTrans, srcTrans );
	}

	/* Return the transition that resulted from the cross. */
	return retTrans;
}

TransAp *FsmAp::copyTransForExpansion( StateAp *from, TransAp *srcTrans )
{
	/* This is the dup without the attach. */
	TransCondAp *newTrans = new TransCondAp();
	newTrans->condSpace = srcTrans->condSpace;

	if ( srcTrans->plain() ) {
		TransDataAp *srcData = srcTrans->tdap();
		CondAp *newCond = new CondAp( newTrans );
		newCond->key = 0;

		attachTrans( srcData->fromState, srcData->toState, newCond );
				
		/* Call the user callback to add in the original source transition. */
		//addInTrans( newCond, srcData );

		/* Not a copy of ourself, get the functions and priorities. */
		newCond->lmActionTable.setActions( srcData->lmActionTable );
		newCond->actionTable.setActions( srcData->actionTable );
		newCond->priorTable.setPriors( srcData->priorTable );

		newTrans->condList.append( newCond );
	}
	else {
		for ( CondList::Iter sc = srcTrans->tcap()->condList; sc.lte(); sc++ ) {
			/* Sub-transition for conditions. */
			CondAp *newCond = new CondAp( newTrans );
			newCond->key = sc->key;

			attachTrans( sc->fromState, sc->toState, newCond );
					
			/* Call the user callback to add in the original source transition. */
			addInTrans( newCond, sc.ptr );

			newTrans->condList.append( newCond );
		}
	}

	/* Set up the transition's keys and append to the dest list. */
	newTrans->lowKey = srcTrans->lowKey;
	newTrans->highKey = srcTrans->highKey;

	return newTrans;
}

void FsmAp::freeEffectiveTrans( TransAp *trans )
{
	for ( CondList::Iter sc = trans->tcap()->condList; sc.lte(); ) {
		CondList::Iter next = sc.next();
		detachTrans( sc->fromState, sc->toState, sc );
		delete sc;
		sc = next;
	}
	delete trans;
}

TransDataAp *FsmAp::crossTransitionsBothPlain( StateAp *from,
		TransDataAp *destTrans, TransDataAp *srcTrans )
{
	/* Neither have cond space and no expansion took place. Cross them. */
	TransDataAp *retTrans;

	/* Compare the priority of the dest and src transitions. */
	int compareRes = comparePrior( destTrans->priorTable, srcTrans->priorTable );
	if ( compareRes < 0 ) {
		/* Src trans has a higher priority than dest, src overwrites dest.
		 * Detach dest and return a copy of src. */
		detachTrans( from, destTrans->toState, destTrans );
		retTrans = dupTransData( from, srcTrans );
	}
	else if ( compareRes > 0 ) {
		/* The dest trans has a higher priority, use dest. */
		retTrans = destTrans;
	}
	else {
		/* Src trans and dest trans have the same priority, they must be merged. */
		retTrans = mergeTrans( from, destTrans, srcTrans );
	}

	/* Return the transition that resulted from the cross. */
	return retTrans;
}

/* Find the trans with the higher priority. If src is lower priority then dest then
 * src is ignored. If src is higher priority than dest, then src overwrites dest. If
 * the priorities are equal, then they are merged. */
TransAp *FsmAp::crossTransitions( StateAp *from,
		TransAp *destTrans, TransAp *srcTrans )
{
	// cerr << __PRETTY_FUNCTION__ << endl;

	if ( destTrans->plain() && srcTrans->plain() ) {
		/* Return the transition that resulted from the cross. */
		return crossTransitionsBothPlain( from,
				destTrans->tdap(), srcTrans->tdap() );
	}
	else {
		/* At least one is non-empty. Target is non-empty. Need to work in
		 * condition spaced. */
		CondSpace *mergedSpace = expandCondSpace( destTrans, srcTrans );

		/* If the dest state cond space does not equal the merged, we have to
		 * rewrite it. If the src state cond space does not equal, we have to
		 * copy it. */

		TransAp *effSrcTrans = srcTrans;
		
		if ( srcTrans->condSpace != mergedSpace ) {
			effSrcTrans = copyTransForExpansion( from, srcTrans );
			CondSpace *orig = effSrcTrans->condSpace;
			effSrcTrans->condSpace = mergedSpace;
			expandConds( from, effSrcTrans, orig, mergedSpace );
		}

		if ( destTrans->condSpace != mergedSpace ) {
			/* Make the transition into a conds transition. If dest is a plain
			 * transition, we have to replace it with a conds transition. */
			if ( destTrans->plain() )
				destTrans = convertToCondAp( from, destTrans->tdap() );

			/* Now expand the dest. */
			CondSpace *orig = destTrans->condSpace;
			destTrans->condSpace = mergedSpace;
			expandConds( from, destTrans, orig, mergedSpace );
		}

		/* The destination list. */
		CondList destList;

		/* Set up an iterator to stop at breaks. */
		ValPairIter< PiList<CondAp> > outPair( destTrans->tcap()->condList,
				effSrcTrans->tcap()->condList );
		for ( ; !outPair.end(); outPair++ ) {
			switch ( outPair.userState ) {
			case ValPairIter<CondAp>::RangeInS1: {
				/* The pair iter is the authority on the keys. It may have needed
				 * to break the dest range. */
				CondAp *destCond = outPair.s1Tel.trans;
				destCond->key = outPair.s1Tel.key;
				destList.append( destCond );
				break;
			}
			case ValPairIter<CondAp>::RangeInS2: {
				/* Src range may get crossed with dest's default transition. */
				CondAp *newCond = dupCondTrans( from, destTrans, outPair.s2Tel.trans );

				/* Set up the transition's keys and append to the dest list. */
				newCond->key = outPair.s2Tel.key;
				destList.append( newCond );
				break;
			}
			case ValPairIter<CondAp>::RangeOverlap: {
				/* Exact overlap, cross them. */
				CondAp *newTrans = crossCondTransitions( from, destTrans,
						outPair.s1Tel.trans, outPair.s2Tel.trans );

				/* Set up the transition's keys and append to the dest list. */
				newTrans->key = outPair.s1Tel.key;
				destList.append( newTrans );
				break;
			}}
		}

		/* Abandon the old outList and transfer destList into it. */
		destTrans->tcap()->condList.transfer( destList );

		/* Delete the duplicate. Don't detach anything. */
		if ( srcTrans != effSrcTrans )
			freeEffectiveTrans( effSrcTrans );

		return destTrans;
	}
}

/* Copy the transitions in srcList to the outlist of dest. The srcList should
 * not be the outList of dest, otherwise you would be copying the contents of
 * srcList into itself as it's iterated: bad news. */
void FsmAp::outTransCopy( StateAp *dest, TransAp *srcList )
{
	/* The destination list. */
	TransList destList;

	/* Set up an iterator to stop at breaks. */
	RangePairIter< PiList<TransAp> > outPair( ctx, dest->outList, srcList );
	for ( ; !outPair.end(); outPair++ ) {
		switch ( outPair.userState ) {
		case RangePairIter<TransAp>::RangeInS1: {
			/* The pair iter is the authority on the keys. It may have needed
			 * to break the dest range. */
			TransAp *destTrans = outPair.s1Tel.trans;
			destTrans->lowKey = outPair.s1Tel.lowKey;
			destTrans->highKey = outPair.s1Tel.highKey;
			destList.append( destTrans );
			break;
		}
		case RangePairIter<TransAp>::RangeInS2: {
			/* Src range may get crossed with dest's default transition. */
			TransAp *newTrans = dupTrans( dest, outPair.s2Tel.trans );

			/* Set up the transition's keys and append to the dest list. */
			newTrans->lowKey = outPair.s2Tel.lowKey;
			newTrans->highKey = outPair.s2Tel.highKey;
			destList.append( newTrans );
			break;
		}
		case RangePairIter<TransAp>::RangeOverlap: {
			/* Exact overlap, cross them. */
			TransAp *newTrans = crossTransitions( dest,
				outPair.s1Tel.trans, outPair.s2Tel.trans );

			/* Set up the transition's keys and append to the dest list. */
			newTrans->lowKey = outPair.s1Tel.lowKey;
			newTrans->highKey = outPair.s1Tel.highKey;
			destList.append( newTrans );
			break;
		}
		case RangePairIter<TransAp>::BreakS1: {
			/* Since we are always writing to the dest trans, the dest needs
			 * to be copied when it is broken. The copy goes into the first
			 * half of the break to "break it off". */
			outPair.s1Tel.trans = dupTrans( dest, outPair.s1Tel.trans );
			break;
		}
		case RangePairIter<TransAp>::BreakS2:
			break;
		}
	}

	/* Abandon the old outList and transfer destList into it. */
	dest->outList.transfer( destList );
}

/* Move all the transitions that go into src so that they go into dest.  */
void FsmAp::moveInwardTrans( StateAp *dest, StateAp *src )
{
	/* Do not try to move in trans to and from the same state. */
	assert( dest != src );

	/* If src is the start state, dest becomes the start state. */
	if ( src == startState ) {
		unsetStartState();
		setStartState( dest );
	}

	/* For each entry point into, create an entry point into dest, when the
	 * state is detached, the entry points to src will be removed. */
	for ( EntryIdSet::Iter enId = src->entryIds; enId.lte(); enId++ )
		changeEntry( *enId, dest, src );

	/* Move the transitions in inList. */
	while ( src->inTrans.head != 0 ) {
		/* Get trans and from state. */
		TransDataAp *trans = src->inTrans.head;
		StateAp *fromState = trans->fromState;

		/* Detach from src, reattach to dest. */
		detachTrans( fromState, src, trans );
		attachTrans( fromState, dest, trans );
	}

	/* Move the transitions in inList. */
	while ( src->inCond.head != 0 ) {
		/* Get trans and from state. */
		CondAp *trans = src->inCond.head;
		StateAp *fromState = trans->fromState;

		/* Detach from src, reattach to dest. */
		detachTrans( fromState, src, trans );
		attachTrans( fromState, dest, trans );
	}

	/* Move inward nfa links. */
	if ( src->nfaIn != 0 ) {
		while ( src->nfaIn->head != 0 ) {
			NfaTrans *trans = src->nfaIn->head;
			StateAp *fromState = trans->fromState;

			detachFromNfa( fromState, src, trans );
			attachToNfa( fromState, dest, trans );
		}
	}
}
