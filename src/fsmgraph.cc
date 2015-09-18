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

using std::cout;
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

	if ( destState->nfaOut != 0 ) {
		for ( NfaTransList::Iter na = *destState->nfaOut; na.lte(); na++ ) {
			na->popAction.setActions( srcState->outActionTable );

			na->popCondSpace = srcState->outCondSpace;
			na->popCondKeys = srcState->outCondKeys;

			na->priorTable.setPriors( srcState->outPriorTable );
		}
	}
}

/* Kleene star operator. Makes this machine the kleene star of itself. Any
 * transitions made going out of the machine and back into itself will be
 * notified that they are leaving transitions by having the leavingFromState
 * callback invoked. */
void FsmAp::starOp( )
{
	/* Turn on misfit accounting to possibly catch the old start state. */
	setMisfitAccounting( true );

	/* Create the new new start state. It will be set final after the merging
	 * of the final states with the start state is complete. */
	StateAp *prevStartState = startState;
	unsetStartState();
	setStartState( addState() );

	/* Merge the new start state with the old one to isolate it. */
	mergeStates( startState, prevStartState );

	/* Merge the start state into all final states. Except the start state on
	 * the first pass. If the start state is set final we will be doubling up
	 * its transitions, which will get transfered to any final states that
	 * follow it in the final state set. This will be determined by the order
	 * of items in the final state set. To prevent this we just merge with the
	 * start on a second pass. */
	for ( StateSet::Iter st = finStateSet; st.lte(); st++ ) {
		if ( *st != startState )
			mergeStatesLeaving( *st, startState );
	}

	/* Now it is safe to merge the start state with itself (provided it
	 * is set final). */
	if ( startState->isFinState() )
		mergeStatesLeaving( startState, startState );

	/* Now ensure the new start state is a final state. */
	setFinState( startState );

	/* Fill in any states that were newed up as combinations of others. */
	fillInStates();

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
		isolateStartState();
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
	isolateStartState();
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
		mergeStatesLeaving( state, otherStartState );

		/* If the former final state was not reset final then we must clear
		 * the state's out trans data. If it got reset final then it gets to
		 * keep its out trans data. This must be done before fillInStates gets
		 * called to prevent the data from being sourced. */
		if ( ! state->isFinState() )
			clearOutData( state );
	}

	/* Fill in any new states made from merging. */
	fillInStates();

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
	for ( PriorTable::Iter g = other->startState->guardedInTable; g.lte(); g++ ) {
		allTransPrior( 0, g->desc );
		other->allTransPrior( 0, g->desc->other );
	}

	/* Assert same signedness and return graph concatenation op. */
	assert( ctx == other->ctx );
	doConcat( other, 0, false );
}

void FsmAp::doOr( FsmAp *other )
{
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
	mergeStateList( startState, startStateSet.data, startStateSet.length() );

	/* Fill in any new states made from merging. */
	fillInStates();
}

/* Unions other with this machine. Other is deleted. */
void FsmAp::unionOp( FsmAp *other )
{
	assert( ctx == other->ctx );

	ctx->unionOp = true;

	setFinBits( STB_GRAPH1 );
	other->setFinBits( STB_GRAPH2 );

	/* Turn on misfit accounting for both graphs. */
	setMisfitAccounting( true );
	other->setMisfitAccounting( true );

	/* Call Worker routine. */
	doOr( other );

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );

	ctx->unionOp = false;
	unsetFinBits( STB_BOTH );
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

void FsmAp::shadowReadWriteStates()
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
						mergeStates( shadow, targ );
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

void FsmAp::resolveEpsilonTrans()
{
	/* Walk the state list and invoke recursive worker on each state. */
	for ( StateList::Iter st = stateList; st.lte(); st++ )
		epsilonFillEptVectFrom( st, st, false );

	/* Prevent reading from and writing to of the same state. */
	shadowReadWriteStates();

	/* For all states that have epsilon transitions out, draw the transitions,
	 * clear the epsilon transitions. */
	for ( StateList::Iter st = stateList; st.lte(); st++ ) {
		/* If there is a state vector, then create the pre-merge state. */
		if ( st->eptVect != 0 ) {
			/* Merge all the epsilon targets into the state. */
			for ( EptVect::Iter ept = *st->eptVect; ept.lte(); ept++ ) {
				if ( ept->leaving )
					mergeStatesLeaving( st, ept->targ );
				else
					mergeStates( st, ept->targ );
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
	setMisfitAccounting( true );

	for ( StateList::Iter st = stateList; st.lte(); st++ )
		st->owningGraph = 0;

	/* Perform merges. */
	resolveEpsilonTrans();

	/* Epsilons can caused merges which leave behind unreachable states. */
	fillInStates();

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
		mergeStateList( newStart, stateSet.data, stateSet.length() );
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
	resolveEpsilonTrans();

	/* Invoke the relinquish final callback on any states that did not get
	 * final state status back. */
	for ( StateSet::Iter st = finStateSetCopy; st.lte(); st++ ) {
		if ( !((*st)->stateBits & STB_ISFINAL) )
			clearOutData( *st );
	}

	/* Fill in any new states made from merging. */
	fillInStates();

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
				mergeStates( newEntry, prevEntry[en].value );

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
	mergeStates( startState, prevStartState );

	/* Stfil and stateDict will be empty because the merging of the old start
	 * state into the new one will not have any conflicting transitions. */
	assert( stateDict.treeSize == 0 );
	assert( nfaList.length() == 0 );

	/* The old start state may be unreachable. Remove the misfits and turn off
	 * misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );
}

StateAp *FsmAp::dupStartState( )
{
	StateAp *dup = addState();
	mergeStates( dup, startState );
	return dup;
}

#if 0
void FsmAp::logOutCondTransfer( StateAp *destState )
{
	std::cerr << "out cond transfer" << std::endl;
	CondSpace *condSpace = destState->outCondSpace;
	for ( CondKeySet::Iter cvi = destState->outCondKeys; cvi.lte(); cvi++ ) {

		std::cerr << " ";
		for ( CondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			int condVals = *cvi;
			bool set = condVals & (1 << csi.pos());
			if ( !set )
				std::cerr << "!";
			(*csi)->actionName( std::cerr );
			if ( !csi.last() )
				std::cerr << ", ";
		}

		std::cerr << std::endl;
	}
}
#endif

/* A state merge which represents the drawing in of leaving transitions.  If
 * there is any out data then we duplicate the source state, transfer the out
 * data, then merge in the state. The new state will be reaped because it will
 * not be given any in transitions. */
void FsmAp::mergeStatesLeaving( StateAp *destState, StateAp *srcState )
{
	if ( !hasOutData( destState ) ) {
		/* Perform the merge, indicating we are leaving, which will affect how
		 * out conds are merged. */
		mergeStates( destState, srcState, true );
	}
	else {
		/* Dup the source state. */
		StateAp *ssMutable = addState();
		mergeStates( ssMutable, srcState );

		/* Do out data transfer (and out condition embedding). */
		transferOutData( ssMutable, destState );

		if ( destState->outCondSpace != 0 ) {

			doEmbedCondition( ssMutable, destState->outCondSpace->condSet,
					destState->outCondKeys );
		}

		/* Now we merge with dest, setting leaving = true. This dictates how
		 * out conditions should be merged. */
		mergeStates( destState, ssMutable, true );
	}
}

void FsmAp::checkEpsilonRegularInteraction( const PriorTable &t1, const PriorTable &t2 )
{
	for ( PriorTable::Iter pd1 = t1; pd1.lte(); pd1++ ) {
		for ( PriorTable::Iter pd2 = t2; pd2.lte(); pd2++ ) {
			if ( pd1->desc->key == pd2->desc->key ) {
				if ( pd1->desc->priority < pd2->desc->priority ) {
					if ( ctx->nfaTermCheck && pd1->desc->guarded )
						throw PriorInteraction( pd1->desc->guardId );
				}
				else if ( pd1->desc->priority > pd2->desc->priority ) {
					if ( ctx->nfaTermCheck && pd1->desc->guarded )
						throw PriorInteraction( pd1->desc->guardId );
				}
			}
		}
	}
}

void FsmAp::mergeStateProperties( StateAp *destState, StateAp *srcState )
{
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
		destState->errActionTable.setActions( ErrActionTable( srcState->errActionTable ) );
		destState->eofActionTable.setActions( ActionTable( srcState->eofActionTable ) );

		/* Not touching guarded-in table or out conditions. Probably should
		 * leave some of the above alone as well. */
	}
	else {
		/* Get the epsilons, out priorities. */
		destState->epsilonTrans.append( srcState->epsilonTrans );
		destState->outPriorTable.setPriors( srcState->outPriorTable );

		/* Get all actions. */
		destState->toStateActionTable.setActions( srcState->toStateActionTable );
		destState->fromStateActionTable.setActions( srcState->fromStateActionTable );
		destState->outActionTable.setActions( srcState->outActionTable );
		destState->errActionTable.setActions( srcState->errActionTable );
		destState->eofActionTable.setActions( srcState->eofActionTable );
		destState->guardedInTable.setPriors( srcState->guardedInTable );

	}


}

void FsmAp::mergeStateBits( StateAp *destState, StateAp *srcState )
{
	/* Get bits and final state status. Note in the above code we depend on the
	 * original final state status being present. */
	destState->stateBits |= ( srcState->stateBits & ~STB_ISFINAL );
	if ( srcState->isFinState() )
		setFinState( destState );
}

void FsmAp::mergeNfaTransions( StateAp *destState, StateAp *srcState )
{
	/* Copy in any NFA transitions. */
	if ( srcState->nfaOut != 0 ) {
		if ( destState->nfaOut == 0 )
			destState->nfaOut = new NfaTransList;

		for ( NfaTransList::Iter nt = *srcState->nfaOut; nt.lte(); nt++ ) {
			NfaTrans *trans = new NfaTrans(
					nt->pushTable, nt->restoreTable,
					nt->popCondSpace, nt->popCondKeys,
					nt->popAction, nt->popTest, nt->order );

			destState->nfaOut->append( trans );
			attachToNfa( destState, nt->toState, trans );
		}
	}
}

void FsmAp::checkPriorInteractions( StateAp *destState, StateAp *srcState )
{
	/* Run a check on priority interactions between epsilon transitions and
	 * regular transitions. This can't be used to affect machine construction,
	 * only to check for priority guards. */
	if ( destState->nfaOut != 0 ) {
		for ( NfaTransList::Iter nt = *destState->nfaOut; nt.lte(); nt++ ) {
			for ( TransList::Iter trans = destState->outList; trans.lte(); trans++ ) {
				if ( trans->plain() ) {
					checkEpsilonRegularInteraction(
							trans->tdap()->priorTable, nt->priorTable );
				}
				else {
					for ( CondList::Iter cond = trans->tcap()->condList;
							cond.lte(); cond++ )
					{
						checkEpsilonRegularInteraction(
								cond->priorTable, nt->priorTable );

					}
				}
			}
		}
	}
}

void FsmAp::mergeStates( StateAp *destState, StateAp *srcState, bool leaving )
{
	/* Transitions. */
	outTransCopy( destState, srcState->outList.head );

	/* Properties such as out data, to/from actions. */
	mergeStateProperties( destState, srcState );

	/* Merge out conditions, depends on the operation (leaving or not). */
	mergeOutConds( destState, srcState, leaving );

	/* State bits, including final state stats. Out conds depnds on this
	 * happening after. */
	mergeStateBits( destState, srcState );

	/* Draw in the NFA transitions. */
	mergeNfaTransions( destState, srcState );

	/* Hacked in check for priority interactions, allowing detection of some
	 * bad situations. */
	checkPriorInteractions( destState, srcState );
}

void FsmAp::mergeStateList( StateAp *destState, 
		StateAp **srcStates, int numSrc )
{
	for ( int s = 0; s < numSrc; s++ )
		mergeStates( destState, srcStates[s] );
}


void FsmAp::fillInStates()
{
	/* Merge any states that are awaiting merging. This will likey cause other
	 * states to be added to the NFA list. */
	while ( nfaList.length() > 0 ) {
		StateAp *state = nfaList.head;

		StateSet *stateSet = &state->stateDictEl->stateSet;
		mergeStateList( state, stateSet->data, stateSet->length() );

		for ( StateSet::Iter s = *stateSet; s.lte(); s++ )
			detachStateDict( state, *s );

		nfaList.detach( state );
	}

	/* The NFA list is empty at this point. There are no state sets we need to
	 * preserve. */

	/* Disassociated state dict elements from states. */
	for ( StateDict::Iter sdi = stateDict; sdi.lte(); sdi++ )
		sdi->targState->stateDictEl = 0;

	/* Delete all the state dict elements. */
	stateDict.empty();
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

