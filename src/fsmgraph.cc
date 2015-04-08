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

#include <assert.h>
#include <iostream>

#include "fsmgraph.h"
#include "mergesort.h"
#include "parsedata.h"

using std::cerr;
using std::endl;

/* Make a new state. The new state will be put on the graph's
 * list of state. The new state can be created final or non final. */
StateAp *FsmAp::addState()
{
	/* Make the new state to return. */
	StateAp *state = new StateAp();

	if ( misfitAccounting ) {
		/* Create the new state on the misfit list. All states are created
		 * with no foreign in transitions. */
		misfitList.append( state );
	}
	else {
		/* Create the new state. */
		stateList.append( state );
	}

	if ( ctx->stateLimit > 0 ) {
		long total = misfitList.length() + stateList.length();
		if ( total > ctx->stateLimit )
			throw TooManyStates();
	}

	return state;
}

/* Construct an FSM that is the concatenation of an array of characters. A new
 * machine will be made that has len+1 states with one transition between each
 * state for each integer in str. IsSigned determines if the integers are to
 * be considered as signed or unsigned ints. */
void FsmAp::concatFsm( Key *str, int len )
{
	/* Make the first state and set it as the start state. */
	StateAp *last = addState();
	setStartState( last );

	/* Attach subsequent states. */
	for ( int i = 0; i < len; i++ ) {
		StateAp *newState = addState();
		attachNewTrans( last, newState, str[i], str[i] );
		last = newState;
	}

	/* Make the last state the final state. */
	setFinState( last );
}

/* Case insensitive version of concatFsm. */
void FsmAp::concatFsmCI( Key *str, int len )
{
	/* Make the first state and set it as the start state. */
	StateAp *last = addState();
	setStartState( last );

	/* Attach subsequent states. */
	for ( int i = 0; i < len; i++ ) {
		StateAp *newState = addState();

		KeySet keySet( ctx->keyOps );
		if ( str[i].isLower() )
			keySet.insert( str[i].toUpper() );
		if ( str[i].isUpper() )
			keySet.insert( str[i].toLower() );
		keySet.insert( str[i] );

		for ( int i = 0; i < keySet.length(); i++ )
			attachNewTrans( last, newState, keySet[i], keySet[i] );

		last = newState;
	}

	/* Make the last state the final state. */
	setFinState( last );
}

/* Construct a machine that matches one character.  A new machine will be made
 * that has two states with a single transition between the states. IsSigned
 * determines if the integers are to be considered as signed or unsigned ints. */
void FsmAp::concatFsm( Key chr )
{
	/* Two states first start, second final. */
	setStartState( addState() );

	StateAp *end = addState();
	setFinState( end );

	/* Attach on the character. */
	attachNewTrans( startState, end, chr, chr );
}

/* Construct a machine that matches any character in set.  A new machine will
 * be made that has two states and len transitions between the them. The set
 * should be ordered correctly accroding to KeyOps and should not contain
 * any duplicates. */
void FsmAp::orFsm( Key *set, int len )
{
	/* Two states first start, second final. */
	setStartState( addState() );

	StateAp *end = addState();
	setFinState( end );

	for ( int i = 1; i < len; i++ )
		assert( ctx->keyOps->lt( set[i-1], set[i] ) );

	/* Attach on all the integers in the given string of ints. */
	for ( int i = 0; i < len; i++ )
		attachNewTrans( startState, end, set[i], set[i] );
}

/* Construct a machine that matches a range of characters.  A new machine will
 * be made with two states and a range transition between them. The range will
 * match any characters from low to high inclusive. Low should be less than or
 * equal to high otherwise undefined behaviour results.  IsSigned determines
 * if the integers are to be considered as signed or unsigned ints. */
void FsmAp::rangeFsm( Key low, Key high )
{
	/* Two states first start, second final. */
	setStartState( addState() );

	StateAp *end = addState();
	setFinState( end );

	/* Attach using the range of characters. */
	attachNewTrans( startState, end, low, high );
}

/* Construct a machine that a repeated range of characters.  */
void FsmAp::rangeStarFsm( Key low, Key high)
{
	/* One state which is final and is the start state. */
	setStartState( addState() );
	setFinState( startState );

	/* Attach start to start using range of characters. */
	attachNewTrans( startState, startState, low, high );
}

/* Construct a machine that matches the empty string.  A new machine will be
 * made with only one state. The new state will be both a start and final
 * state. IsSigned determines if the machine has a signed or unsigned
 * alphabet. Fsm operations must be done on machines with the same alphabet
 * signedness. */
void FsmAp::lambdaFsm( )
{
	/* Give it one state with no transitions making it
	 * the start state and final state. */
	setStartState( addState() );
	setFinState( startState );
}

/* Construct a machine that matches nothing at all. A new machine will be
 * made with only one state. It will not be final. */
void FsmAp::emptyFsm( )
{
	/* Give it one state with no transitions making it
	 * the start state and final state. */
	setStartState( addState() );
}

void FsmAp::transferOutData( StateAp *destState, StateAp *srcState )
{
	for ( TransList::Iter trans = destState->outList; trans.lte(); trans++ ) {
		if ( trans->plain() ) {
			if ( trans->tdap()->toState != 0 ) {
				/* Get the actions data from the outActionTable. */
				trans->tdap()->actionTable.setActions( srcState->outActionTable );

				/* Get the priorities from the outPriorTable. */
				trans->tdap()->priorTable.setPriors( srcState->outPriorTable );
			}
		}
		else {
			for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) {
				if ( cond->toState != 0 ) {
					/* Get the actions data from the outActionTable. */
					cond->actionTable.setActions( srcState->outActionTable );

					/* Get the priorities from the outPriorTable. */
					cond->priorTable.setPriors( srcState->outPriorTable );
				}
			}
		}
	}
}

/* Kleene star operator. Makes this machine the kleene star of itself. Any
 * transitions made going out of the machine and back into itself will be
 * notified that they are leaving transitions by having the leavingFromState
 * callback invoked. */
void FsmAp::starOp( )
{
	/* For the merging process. */
	MergeData md;

	/* Turn on misfit accounting to possibly catch the old start state. */
	setMisfitAccounting( true );

	/* Create the new new start state. It will be set final after the merging
	 * of the final states with the start state is complete. */
	StateAp *prevStartState = startState;
	unsetStartState();
	setStartState( addState() );

	/* Merge the new start state with the old one to isolate it. */
	mergeStates( md, startState, prevStartState );

	/* Merge the start state into all final states. Except the start state on
	 * the first pass. If the start state is set final we will be doubling up
	 * its transitions, which will get transfered to any final states that
	 * follow it in the final state set. This will be determined by the order
	 * of items in the final state set. To prevent this we just merge with the
	 * start on a second pass. */
	for ( StateSet::Iter st = finStateSet; st.lte(); st++ ) {
		if ( *st != startState )
			mergeStatesLeaving( md, *st, startState );
	}

	/* Now it is safe to merge the start state with itself (provided it
	 * is set final). */
	if ( startState->isFinState() )
		mergeStatesLeaving( md, startState, startState );

	/* Now ensure the new start state is a final state. */
	setFinState( startState );

	/* Fill in any states that were newed up as combinations of others. */
	fillInStates( md );

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );
}

void FsmAp::repeatOp( int times )
{
	/* Must be 1 and up. 0 produces null machine and requires deleting this. */
	assert( times > 0 );

	/* A repeat of one does absolutely nothing. */
	if ( times == 1 )
		return;

	/* Make a machine to make copies from. */
	FsmAp *copyFrom = new FsmAp( *this );

	/* Concatentate duplicates onto the end up until before the last. */
	for ( int i = 1; i < times-1; i++ ) {
		FsmAp *dup = new FsmAp( *copyFrom );
		doConcat( dup, 0, false );
	}

	/* Now use the copyFrom on the end. */
	doConcat( copyFrom, 0, false );
}

void FsmAp::optionalRepeatOp( int times )
{
	/* Must be 1 and up. 0 produces null machine and requires deleting this. */
	assert( times > 0 );

	/* A repeat of one optional merely allows zero string. */
	if ( times == 1 ) {
		setFinState( startState );
		return;
	}

	/* Make a machine to make copies from. */
	FsmAp *copyFrom = new FsmAp( *this );

	/* The state set used in the from end of the concatentation. Starts with
	 * the initial final state set, then after each concatenation, gets set to
	 * the the final states that come from the the duplicate. */
	StateSet lastFinSet( finStateSet );

	/* Set the initial state to zero to allow zero copies. */
	setFinState( startState );

	/* Concatentate duplicates onto the end up until before the last. */
	for ( int i = 1; i < times-1; i++ ) {
		/* Make a duplicate for concating and set the fin bits to graph 2 so we
		 * can pick out it's final states after the optional style concat. */
		FsmAp *dup = new FsmAp( *copyFrom );
		dup->setFinBits( STB_GRAPH2 );
		doConcat( dup, &lastFinSet, true );

		/* Clear the last final state set and make the new one by taking only
		 * the final states that come from graph 2.*/
		lastFinSet.empty();
		for ( int i = 0; i < finStateSet.length(); i++ ) {
			/* If the state came from graph 2, add it to the last set and clear
			 * the bits. */
			StateAp *fs = finStateSet[i];
			if ( fs->stateBits & STB_GRAPH2 ) {
				lastFinSet.insert( fs );
				fs->stateBits &= ~STB_GRAPH2;
			}
		}
	}

	/* Now use the copyFrom on the end, no bits set, no bits to clear. */
	doConcat( copyFrom, &lastFinSet, true );
}


/* Fsm concatentation worker. Supports treating the concatentation as optional,
 * which essentially leaves the final states of machine one as final. */
void FsmAp::doConcat( FsmAp *other, StateSet *fromStates, bool optional )
{
	/* For the merging process. */
	StateSet finStateSetCopy, startStateSet;
	MergeData md;

	/* Turn on misfit accounting for both graphs. */
	setMisfitAccounting( true );
	other->setMisfitAccounting( true );

	/* Get the other's start state. */
	StateAp *otherStartState = other->startState;

	/* Unset other's start state before bringing in the entry points. */
	other->unsetStartState();

	/* Bring in the rest of other's entry points. */
	copyInEntryPoints( other );
	other->entryPoints.empty();

	/* Bring in other's states into our state lists. */
	stateList.append( other->stateList );
	misfitList.append( other->misfitList );

	/* If from states is not set, then get a copy of our final state set before
	 * we clobber it and use it instead. */
	if ( fromStates == 0 ) {
		finStateSetCopy = finStateSet;
		fromStates = &finStateSetCopy;
	}

	/* Unset all of our final states and get the final states from other. */
	if ( !optional )
		unsetAllFinStates();
	finStateSet.insert( other->finStateSet );
	
	/* Since other's lists are empty, we can delete the fsm without
	 * affecting any states. */
	delete other;

	/* Merge our former final states with the start state of other. */
	for ( int i = 0; i < fromStates->length(); i++ ) {
		StateAp *state = fromStates->data[i];

		/* Merge the former final state with other's start state. */
		mergeStatesLeaving( md, state, otherStartState );

		/* If the former final state was not reset final then we must clear
		 * the state's out trans data. If it got reset final then it gets to
		 * keep its out trans data. This must be done before fillInStates gets
		 * called to prevent the data from being sourced. */
		if ( ! state->isFinState() )
			clearOutData( state );
	}

	/* Fill in any new states made from merging. */
	fillInStates( md );

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );
}

/* Concatenates other to the end of this machine. Other is deleted.  Any
 * transitions made leaving this machine and entering into other are notified
 * that they are leaving transitions by having the leavingFromState callback
 * invoked. */
void FsmAp::concatOp( FsmAp *other )
{
	/* Assert same signedness and return graph concatenation op. */
	assert( ctx == other->ctx );
	doConcat( other, 0, false );
}

void FsmAp::nfaRepeatOp( Action *action1, Action *action2, Action *action3 )
{
	/*
	 * First Concat.
	 */
	StateSet origFinals = finStateSet;

	/* Get the other's start state. */
	StateAp *origStartState = startState;
	StateAp *repStartState = dupStartState();


	StateAp *newStart = addState();

	newStart->nfaOut = new StateSet;
	newStart->nfaOut->insert( origStartState );
	attachToNfa( newStart, origStartState );

	StateAp *newFinal = addState();

	for ( StateSet::Iter orig = origFinals; orig.lte(); orig++ ) {
		unsetFinState( *orig );

		StateAp *repl = addState();
		moveInwardTrans( repl, *orig );
		
		repl->nfaOut = new StateSet;
		repl->nfaOut->insert( repStartState );
		repl->nfaOut->insert( *orig );
		repl->nfaOut->insert( newFinal );

		for ( StateSet::Iter s = *repl->nfaOut; s.lte(); s++ )
			attachToNfa( repl, *s );
	}

	origStartState->fromStateActionTable.setAction( 0, action1 );
	newFinal->fromStateActionTable.setAction( 0, action2 );
	repStartState->fromStateActionTable.setAction( 0, action3 );

	unsetStartState();
	setStartState( newStart );
	setFinState( newFinal );
}

void FsmAp::doOr( FsmAp *other )
{
	/* For the merging process. */
	MergeData md;

	/* Build a state set consisting of both start states */
	StateSet startStateSet;
	startStateSet.insert( startState );
	startStateSet.insert( other->startState );

	/* Both of the original start states loose their start state status. */
	unsetStartState();
	other->unsetStartState();

	/* Bring in the rest of other's entry points. */
	copyInEntryPoints( other );
	other->entryPoints.empty();

	/* Merge the lists. This will move all the states from other
	 * into this. No states will be deleted. */
	stateList.append( other->stateList );
	misfitList.append( other->misfitList );

	/* Move the final set data from other into this. */
	finStateSet.insert(other->finStateSet);
	other->finStateSet.empty();

	/* Since other's list is empty, we can delete the fsm without
	 * affecting any states. */
	delete other;

	/* Create a new start state. */
	setStartState( addState() );

	/* Merge the start states. */
	mergeStates( md, startState, startStateSet.data, startStateSet.length() );

	/* Fill in any new states made from merging. */
	fillInStates( md );
}

/* Unions other with this machine. Other is deleted. */
void FsmAp::unionOp( FsmAp *other )
{
	assert( ctx == other->ctx );

	/* Turn on misfit accounting for both graphs. */
	setMisfitAccounting( true );
	other->setMisfitAccounting( true );

	/* Call Worker routine. */
	doOr( other );

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );
}

void FsmAp::nfaFillInStates( MergeData &md )
{
	long count = nfaList.length();

	/* Merge any states that are awaiting merging. This will likey cause
	 * other states to be added to the stfil list. */
	while ( nfaList.length() > 0 && count-- ) {
		StateAp *state = nfaList.head;

		StateSet *stateSet = &state->stateDictEl->stateSet;
		nfaMergeStates( md, state, stateSet->data, stateSet->length() );

		for ( StateSet::Iter s = *stateSet; s.lte(); s++ )
			detachFromNfa( state, *s );

		nfaList.detach( state );

		// std::cout << "misfit-list-len: " << misfitList.length() << std::endl;
	}
}

/* Unions other with this machine. Other is deleted. */
void FsmAp::nfaUnionOp( FsmAp **others, int n, long rounds )
{
	for ( int o = 0; o < n; o++ )
		assert( ctx == others[o]->ctx );

	/* Not doing misfit accounting here. If we wanted to, it would need to be
	 * made nfa-compatibile. */

	/* For the merging process. */
	MergeData md;

	/* Build a state set consisting of both start states */
	StateSet startStateSet;
	startStateSet.insert( startState );
	for ( int o = 0; o < n; o++ )
		startStateSet.insert( others[o]->startState );

	/* Both of the original start states loose their start state status. */
	unsetStartState();
	for ( int o = 0; o < n; o++ )
		others[o]->unsetStartState();

	/* Bring in the rest of other's entry points. */
	for ( int o = 0; o < n; o++ ) {
		copyInEntryPoints( others[o] );
		others[o]->entryPoints.empty();
	}

	for ( int o = 0; o < n; o++ ) {
		/* Merge the lists. This will move all the states from other
		 * into this. No states will be deleted. */
		stateList.append( others[o]->stateList );
		misfitList.append( others[o]->misfitList );
		// nfaList.append( others[o]->nfaList );
	}

	for ( int o = 0; o < n; o++ ) {
		/* Move the final set data from other into this. */
		finStateSet.insert( others[o]->finStateSet );
		others[o]->finStateSet.empty();
	}

	for ( int o = 0; o < n; o++ ) {
		/* Since other's list is empty, we can delete the fsm without
		 * affecting any states. */
		delete others[o];
	}

	/* Create a new start state. */
	setStartState( addState() );

	if ( rounds == 0 ) {
		startState->stateDictEl = new StateDictEl( startStateSet );
		nfaList.append( startState );

		for ( StateSet::Iter s = startStateSet; s.lte(); s++ )
			attachToNfa( startState, *s );
	}
	else {
		/* Merge the start states. */
		std::cout << "nfa-fill-round\t0" << std::endl;
		nfaMergeStates( md, startState, startStateSet.data, startStateSet.length() );

		/* Fill in any new states made from merging. */
		for ( long i = 1; i < rounds && nfaList.length() > 0; i++ ) {
			std::cout << "nfa-fill-round\t" << i << std::endl;
			nfaFillInStates( md );
		}

		/* For any remaining NFA states, remove from the state dict. We need to
		 * keep the state sets. */
		for ( NfaStateList::Iter ns = nfaList; ns.lte(); ns++ )
			md.stateDict.detach( ns->stateDictEl );

		/* Disassociate non-nfa states from their state dicts. */
		for ( StateDict::Iter sdi = md.stateDict; sdi.lte(); sdi++ )
			sdi->targState->stateDictEl = 0;

		/* Delete the state dict elements for non-nfa states. */
		md.stateDict.empty();

		/* Transfer remaining stateDictEl sets to nfaOut. */
		while ( nfaList.length() > 0 ) {
			StateAp *state = nfaList.head;
			state->nfaOut = new StateSet( state->stateDictEl->stateSet );
			delete state->stateDictEl;
			state->stateDictEl = 0;
			nfaList.detach( state );
		}


		long maxStateSetSize = 0;
		long count = nfaList.length();
		for ( NfaStateList::Iter ns = nfaList; ns.lte(); ns++ ) {
			StateSet &stateSet = ns->stateDictEl->stateSet;
			if ( stateSet.length() > maxStateSetSize )
				maxStateSetSize = stateSet.length();
		}

		std::cout << "fill-list\t" << count << std::endl;
		std::cout << "state-dict\t" << md.stateDict.length() << std::endl;
		std::cout << "states\t" << stateList.length() << std::endl;
		std::cout << "max-ss\t" << maxStateSetSize << std::endl;

		removeUnreachableStates();

		std::cout << "post-unreachable\t" << stateList.length() << std::endl;

		minimizePartition2();

		std::cout << "post-min\t" << stateList.length() << std::endl;
		std::cout << std::endl;
	}
}


/* Intersects other with this machine. Other is deleted. */
void FsmAp::intersectOp( FsmAp *other )
{
	assert( ctx == other->ctx );

	/* Turn on misfit accounting for both graphs. */
	setMisfitAccounting( true );
	other->setMisfitAccounting( true );

	/* Set the fin bits on this and other to want each other. */
	setFinBits( STB_GRAPH1 );
	other->setFinBits( STB_GRAPH2 );

	/* Call worker Or routine. */
	doOr( other );

	/* Unset any final states that are no longer to 
	 * be final due to final bits. */
	unsetIncompleteFinals();

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );

	/* Remove states that have no path to a final state. */
	removeDeadEndStates();
}

/* Set subtracts other machine from this machine. Other is deleted. */
void FsmAp::subtractOp( FsmAp *other )
{
	assert( ctx == other->ctx );

	/* Turn on misfit accounting for both graphs. */
	setMisfitAccounting( true );
	other->setMisfitAccounting( true );

	/* Set the fin bits of other to be killers. */
	other->setFinBits( STB_GRAPH1 );

	/* Call worker Or routine. */
	doOr( other );

	/* Unset any final states that are no longer to 
	 * be final due to final bits. */
	unsetKilledFinals();

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );

	/* Remove states that have no path to a final state. */
	removeDeadEndStates();
}

bool FsmAp::inEptVect( EptVect *eptVect, StateAp *state )
{
	if ( eptVect != 0 ) {
		/* Vect is there, walk it looking for state. */
		for ( int i = 0; i < eptVect->length(); i++ ) {
			if ( eptVect->data[i].targ == state )
				return true;
		}
	}
	return false;
}

/* Fill epsilon vectors in a root state from a given starting point. Epmploys
 * a depth first search through the graph of epsilon transitions. */
void FsmAp::epsilonFillEptVectFrom( StateAp *root, StateAp *from, bool parentLeaving )
{
	/* Walk the epsilon transitions out of the state. */
	for ( EpsilonTrans::Iter ep = from->epsilonTrans; ep.lte(); ep++ ) {
		/* Find the entry point, if the it does not resove, ignore it. */
		EntryMapEl *enLow, *enHigh;
		if ( entryPoints.findMulti( *ep, enLow, enHigh ) ) {
			/* Loop the targets. */
			for ( EntryMapEl *en = enLow; en <= enHigh; en++ ) {
				/* Do not add the root or states already in eptVect. */
				StateAp *targ = en->value;
				if ( targ != from && !inEptVect(root->eptVect, targ) ) {
					/* Maybe need to create the eptVect. */
					if ( root->eptVect == 0 )
						root->eptVect = new EptVect();

					/* If moving to a different graph or if any parent is
					 * leaving then we are leaving. */
					bool leaving = parentLeaving || 
							root->owningGraph != targ->owningGraph;

					/* All ok, add the target epsilon and recurse. */
					root->eptVect->append( EptVectEl(targ, leaving) );
					epsilonFillEptVectFrom( root, targ, leaving );
				}
			}
		}
	}
}

void FsmAp::shadowReadWriteStates( MergeData &md )
{
	/* Init isolatedShadow algorithm data. */
	for ( StateList::Iter st = stateList; st.lte(); st++ )
		st->isolatedShadow = 0;

	/* Any states that may be both read from and written to must 
	 * be shadowed. */
	for ( StateList::Iter st = stateList; st.lte(); st++ ) {
		/* Find such states by looping through stateVect lists, which give us
		 * the states that will be read from. May cause us to visit the states
		 * that we are interested in more than once. */
		if ( st->eptVect != 0 ) {
			/* For all states that will be read from. */
			for ( EptVect::Iter ept = *st->eptVect; ept.lte(); ept++ ) {
				/* Check for read and write to the same state. */
				StateAp *targ = ept->targ;
				if ( targ->eptVect != 0 ) {
					/* State is to be written to, if the shadow is not already
					 * there, create it. */
					if ( targ->isolatedShadow == 0 ) {
						StateAp *shadow = addState();
						mergeStates( md, shadow, targ );
						targ->isolatedShadow = shadow;
					}

					/* Write shadow into the state vector so that it is the
					 * state that the epsilon transition will read from. */
					ept->targ = targ->isolatedShadow;
				}
			}
		}
	}
}

void FsmAp::resolveEpsilonTrans( MergeData &md )
{
	/* Walk the state list and invoke recursive worker on each state. */
	for ( StateList::Iter st = stateList; st.lte(); st++ )
		epsilonFillEptVectFrom( st, st, false );

	/* Prevent reading from and writing to of the same state. */
	shadowReadWriteStates( md );

	/* For all states that have epsilon transitions out, draw the transitions,
	 * clear the epsilon transitions. */
	for ( StateList::Iter st = stateList; st.lte(); st++ ) {
		/* If there is a state vector, then create the pre-merge state. */
		if ( st->eptVect != 0 ) {
			/* Merge all the epsilon targets into the state. */
			for ( EptVect::Iter ept = *st->eptVect; ept.lte(); ept++ ) {
				if ( ept->leaving )
					mergeStatesLeaving( md, st, ept->targ );
				else
					mergeStates( md, st, ept->targ );
			}

			/* Clean up the target list. */
			delete st->eptVect;
			st->eptVect = 0;
		}

		/* Clear the epsilon transitions vector. */
		st->epsilonTrans.empty();
	}
}

void FsmAp::epsilonOp()
{
	/* For merging process. */
	MergeData md;

	setMisfitAccounting( true );

	for ( StateList::Iter st = stateList; st.lte(); st++ )
		st->owningGraph = 0;

	/* Perform merges. */
	resolveEpsilonTrans( md );

	/* Epsilons can caused merges which leave behind unreachable states. */
	fillInStates( md );

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );
}

/* Make a new maching by joining together a bunch of machines without making
 * any transitions between them. A negative finalId results in there being no
 * final id. */
void FsmAp::joinOp( int startId, int finalId, FsmAp **others, int numOthers )
{
	for ( int m = 0; m < numOthers; m++ ) {
		assert( ctx == others[m]->ctx );
	}

	/* For the merging process. */
	MergeData md;

	/* Set the owning machines. Start at one. Zero is reserved for the start
	 * and final states. */
	for ( StateList::Iter st = stateList; st.lte(); st++ )
		st->owningGraph = 1;
	for ( int m = 0; m < numOthers; m++ ) {
		for ( StateList::Iter st = others[m]->stateList; st.lte(); st++ )
			st->owningGraph = 2+m;
	}

	/* All machines loose start state status. */
	unsetStartState();
	for ( int m = 0; m < numOthers; m++ )
		others[m]->unsetStartState();
	
	/* Bring the other machines into this. */
	for ( int m = 0; m < numOthers; m++ ) {
		/* Bring in the rest of other's entry points. */
		copyInEntryPoints( others[m] );
		others[m]->entryPoints.empty();

		/* Merge the lists. This will move all the states from other into
		 * this. No states will be deleted. */
		stateList.append( others[m]->stateList );
		assert( others[m]->misfitList.length() == 0 );

		/* Move the final set data from other into this. */
		finStateSet.insert( others[m]->finStateSet );
		others[m]->finStateSet.empty();

		/* Since other's list is empty, we can delete the fsm without
		 * affecting any states. */
		delete others[m];
	}

	/* Look up the start entry point. */
	EntryMapEl *enLow = 0, *enHigh = 0;
	bool findRes = entryPoints.findMulti( startId, enLow, enHigh );
	if ( ! findRes ) {
		/* No start state. Set a default one and proceed with the join. Note
		 * that the result of the join will be a very uninteresting machine. */
		setStartState( addState() );
	}
	else {
		/* There is at least one start state, create a state that will become
		 * the new start state. */
		StateAp *newStart = addState();
		setStartState( newStart );

		/* The start state is in an owning machine class all it's own. */
		newStart->owningGraph = 0;

		/* Create the set of states to merge from. */
		StateSet stateSet;
		for ( EntryMapEl *en = enLow; en <= enHigh; en++ )
			stateSet.insert( en->value );

		/* Merge in the set of start states into the new start state. */
		mergeStates( md, newStart, stateSet.data, stateSet.length() );
	}

	/* Take a copy of the final state set, before unsetting them all. This
	 * will allow us to call clearOutData on the states that don't get
	 * final state status back back. */
	StateSet finStateSetCopy = finStateSet;

	/* Now all final states are unset. */
	unsetAllFinStates();

	if ( finalId >= 0 ) {
		/* Create the implicit final state. */
		StateAp *finState = addState();
		setFinState( finState );

		/* Assign an entry into the final state on the final state entry id. Note
		 * that there may already be an entry on this id. That's ok. Also set the
		 * final state owning machine id. It's in a class all it's own. */
		setEntry( finalId, finState );
		finState->owningGraph = 0;
	}

	/* Hand over to workers for resolving epsilon trans. This will merge states
	 * with the targets of their epsilon transitions. */
	resolveEpsilonTrans( md );

	/* Invoke the relinquish final callback on any states that did not get
	 * final state status back. */
	for ( StateSet::Iter st = finStateSetCopy; st.lte(); st++ ) {
		if ( !((*st)->stateBits & STB_ISFINAL) )
			clearOutData( *st );
	}

	/* Fill in any new states made from merging. */
	fillInStates( md );

	/* Joining can be messy. Instead of having misfit accounting on (which is
	 * tricky here) do a full cleaning. */
	removeUnreachableStates();
}

void FsmAp::globOp( FsmAp **others, int numOthers )
{
	for ( int m = 0; m < numOthers; m++ ) {
		assert( ctx == others[m]->ctx );
	}

	/* All other machines loose start states status. */
	for ( int m = 0; m < numOthers; m++ )
		others[m]->unsetStartState();
	
	/* Bring the other machines into this. */
	for ( int m = 0; m < numOthers; m++ ) {
		/* Bring in the rest of other's entry points. */
		copyInEntryPoints( others[m] );
		others[m]->entryPoints.empty();

		/* Merge the lists. This will move all the states from other into
		 * this. No states will be deleted. */
		stateList.append( others[m]->stateList );
		assert( others[m]->misfitList.length() == 0 );

		/* Move the final set data from other into this. */
		finStateSet.insert( others[m]->finStateSet );
		others[m]->finStateSet.empty();

		/* Since other's list is empty, we can delete the fsm without
		 * affecting any states. */
		delete others[m];
	}
}

void FsmAp::deterministicEntry()
{
	/* For the merging process. */
	MergeData md;

	/* States may loose their entry points, turn on misfit accounting. */
	setMisfitAccounting( true );

	/* Get a copy of the entry map then clear all the entry points. As we
	 * iterate the old entry map finding duplicates we will add the entry
	 * points for the new states that we create. */
	EntryMap prevEntry = entryPoints;
	unsetAllEntryPoints();

	for ( int enId = 0; enId < prevEntry.length(); ) {
		/* Count the number of states on this entry key. */
		int highId = enId;
		while ( highId < prevEntry.length() && prevEntry[enId].key == prevEntry[highId].key )
			highId += 1;

		int numIds = highId - enId;
		if ( numIds == 1 ) {
			/* Only a single entry point, just set the entry. */
			setEntry( prevEntry[enId].key, prevEntry[enId].value );
		}
		else {
			/* Multiple entry points, need to create a new state and merge in
			 * all the targets of entry points. */
			StateAp *newEntry = addState();
			for ( int en = enId; en < highId; en++ )
				mergeStates( md, newEntry, prevEntry[en].value );

			/* Add the new state as the single entry point. */
			setEntry( prevEntry[enId].key, newEntry );
		}

		enId += numIds;
	}

	/* The old start state may be unreachable. Remove the misfits and turn off
	 * misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );
}

/* Unset any final states that are no longer to be final due to final bits. */
void FsmAp::unsetKilledFinals()
{
	/* Duplicate the final state set before we begin modifying it. */
	StateSet fin( finStateSet );

	for ( int s = 0; s < fin.length(); s++ ) {
		/* Check for killing bit. */
		StateAp *state = fin.data[s];
		if ( state->stateBits & STB_GRAPH1 ) {
			/* One final state is a killer, set to non-final. */
			unsetFinState( state );
		}

		/* Clear all killing bits. Non final states should never have had those
		 * state bits set in the first place. */
		state->stateBits &= ~STB_GRAPH1;
	}
}

/* Unset any final states that are no longer to be final due to final bits. */
void FsmAp::unsetIncompleteFinals()
{
	/* Duplicate the final state set before we begin modifying it. */
	StateSet fin( finStateSet );

	for ( int s = 0; s < fin.length(); s++ ) {
		/* Check for one set but not the other. */
		StateAp *state = fin.data[s];
		if ( state->stateBits & STB_BOTH && 
				(state->stateBits & STB_BOTH) != STB_BOTH )
		{
			/* One state wants the other but it is not there. */
			unsetFinState( state );
		}

		/* Clear wanting bits. Non final states should never have had those
		 * state bits set in the first place. */
		state->stateBits &= ~STB_BOTH;
	}
}

/* Ensure that the start state is free of entry points (aside from the fact
 * that it is the start state). If the start state has entry points then Make a
 * new start state by merging with the old one. Useful before modifying start
 * transitions. If the existing start state has any entry points other than the
 * start state entry then modifying its transitions changes more than the start
 * transitions. So isolate the start state by separating it out such that it
 * only has start stateness as it's entry point. */
void FsmAp::isolateStartState( )
{
	/* For the merging process. */
	MergeData md;

	/* Bail out if the start state is already isolated. */
	if ( isStartStateIsolated() )
		return;

	/* Turn on misfit accounting to possibly catch the old start state. */
	setMisfitAccounting( true );

	/* This will be the new start state. The existing start
	 * state is merged with it. */
	StateAp *prevStartState = startState;
	unsetStartState();
	setStartState( addState() );

	/* Merge the new start state with the old one to isolate it. */
	mergeStates( md, startState, prevStartState );

	/* Stfil and stateDict will be empty because the merging of the old start
	 * state into the new one will not have any conflicting transitions. */
	assert( md.stateDict.treeSize == 0 );
	assert( nfaList.length() == 0 );

	/* The old start state may be unreachable. Remove the misfits and turn off
	 * misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );
}

StateAp *FsmAp::dupStartState( )
{
	MergeData md;
	StateAp *dup = addState();
	mergeStates( md, dup, startState );
	return dup;
}

/* A state merge which represents the drawing in of leaving transitions.  If
 * there is any out data then we duplicate the source state, transfer the out
 * data, then merge in the state. The new state will be reaped because it will
 * not be given any in transitions. */
void FsmAp::mergeStatesLeaving( MergeData &md, StateAp *destState, StateAp *srcState )
{
	if ( !hasOutData( destState ) )
		mergeStates( md, destState, srcState );
	else {
		StateAp *ssMutable = addState();
		mergeStates( md, ssMutable, srcState );
		transferOutData( ssMutable, destState );

		for ( OutCondSet::Iter cond = destState->outCondSet; cond.lte(); cond++ )
			embedCondition( md, ssMutable, cond->action, cond->sense );

		mergeStates( md, destState, ssMutable );
	}
}

void FsmAp::mergeStates( MergeData &md, StateAp *destState, 
		StateAp **srcStates, int numSrc )
{
	for ( int s = 0; s < numSrc; s++ )
		mergeStates( md, destState, srcStates[s] );
}

void FsmAp::nfaMergeStates( MergeData &md, StateAp *destState, 
		StateAp **srcStates, int numSrc )
{
	for ( int s = 0; s < numSrc; s++ ) {
		mergeStates( md, destState, srcStates[s] );

		while ( misfitList.length() > 0 ) {
			StateAp *state = misfitList.head;

			if ( state->stateDictEl ) {
				md.stateDict.detach( state->stateDictEl );
				nfaList.detach( state );
			}

			/* Detach and delete. */
			detachState( state );
			misfitList.detach( state );
			delete state;

		}

		//std::cout << "progress: " << (float)s * 100.0 / (float)numSrc << std::endl;
		//std::cout << "misfit-list-length: " << misfitList.length() << std::endl;
		//std::cout << "state-list-length: " << stateList.length() << std::endl;
		//std::cout << std::endl;
	}
}

void FsmAp::mergeStates( MergeData &md, StateAp *destState, StateAp *srcState )
{
	outTransCopy( md, destState, srcState->outList.head );

	/* Get its bits and final state status. */
	destState->stateBits |= ( srcState->stateBits & ~STB_ISFINAL );
	if ( srcState->isFinState() )
		setFinState( destState );

	/* Draw in any properties of srcState into destState. */
	if ( srcState == destState ) {
		/* Duplicate the list to protect against write to source. The
		 * priorities sets are not copied in because that would have no
		 * effect. */
		destState->epsilonTrans.append( EpsilonTrans( srcState->epsilonTrans ) );

		/* Get all actions, duplicating to protect against write to source. */
		destState->toStateActionTable.setActions( 
				ActionTable( srcState->toStateActionTable ) );
		destState->fromStateActionTable.setActions( 
				ActionTable( srcState->fromStateActionTable ) );
		destState->outActionTable.setActions( ActionTable( srcState->outActionTable ) );
		destState->outCondSet.insert( OutCondSet( srcState->outCondSet ) );
		destState->errActionTable.setActions( ErrActionTable( srcState->errActionTable ) );
		destState->eofActionTable.setActions( ActionTable( srcState->eofActionTable ) );
	}
	else {
		/* Get the epsilons, out priorities. */
		destState->epsilonTrans.append( srcState->epsilonTrans );
		destState->outPriorTable.setPriors( srcState->outPriorTable );

		/* Get all actions. */
		destState->toStateActionTable.setActions( srcState->toStateActionTable );
		destState->fromStateActionTable.setActions( srcState->fromStateActionTable );
		destState->outActionTable.setActions( srcState->outActionTable );
		destState->outCondSet.insert( srcState->outCondSet );
		destState->errActionTable.setActions( srcState->errActionTable );
		destState->eofActionTable.setActions( srcState->eofActionTable );

	}

	if ( srcState->nfaOut != 0 ) {
		if ( destState->nfaOut == 0 )
			destState->nfaOut = new StateSet;

		for ( StateSet::Iter nt = *srcState->nfaOut; nt.lte(); nt++ ) {
			if ( destState->nfaOut->insert( *nt ) )
				attachToNfa( destState, *nt );
		}
	}
}

void FsmAp::fillInStates( MergeData &md )
{
	/* Merge any states that are awaiting merging. This will likey cause other
	 * states to be added to the NFA list. */
	while ( nfaList.length() > 0 ) {
		StateAp *state = nfaList.head;

		StateSet *stateSet = &state->stateDictEl->stateSet;
		mergeStates( md, state, stateSet->data, stateSet->length() );

		for ( StateSet::Iter s = *stateSet; s.lte(); s++ )
			detachFromNfa( state, *s );

		nfaList.detach( state );
	}

	/* The NFA list is empty at this point. There are no state sets we need to
	 * preserve. */

	/* Disassociated state dict elements from states. */
	for ( StateDict::Iter sdi = md.stateDict; sdi.lte(); sdi++ )
		sdi->targState->stateDictEl = 0;

	/* Delete all the state dict elements. */
	md.stateDict.empty();
}


/* Check if a machine defines a single character. This is useful in validating
 * ranges and machines to export. */
bool FsmAp::checkSingleCharMachine()
{
	/* Must have two states. */
	if ( stateList.length() != 2 )
		return false;
	/* The start state cannot be final. */
	if ( startState->isFinState() )
		return false;
	/* There should be only one final state. */
	if ( finStateSet.length() != 1 )
		return false;
	/* The final state cannot have any transitions out. */
	if ( finStateSet[0]->outList.length() != 0 )
		return false;
	/* The start state should have only one transition out. */
	if ( startState->outList.length() != 1 )
		return false;
	/* The singe transition out of the start state should not be a range. */
	TransAp *startTrans = startState->outList.head;
	if ( ctx->keyOps->ne( startTrans->lowKey, startTrans->highKey ) )
		return false;
	return true;
}

