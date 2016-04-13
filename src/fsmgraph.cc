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
#include "action.h"

using std::endl;
	
Action::~Action()
{
	/* If we were created by substitution of another action then we don't own the inline list. */
	if ( substOf == 0 && inlineList != 0 ) {
		inlineList->empty();
		delete inlineList;
		inlineList = 0;
	}
}

InlineItem::~InlineItem()
{
	if ( children != 0 ) {
		children->empty();
		delete children;
	}
}

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

	return state;
}

/* Construct an FSM that is the concatenation of an array of characters. A new
 * machine will be made that has len+1 states with one transition between each
 * state for each integer in str. IsSigned determines if the integers are to
 * be considered as signed or unsigned ints. */
FsmAp *FsmAp::concatFsm( FsmCtx *ctx, Key *str, int len )
{
	FsmAp *fsm = new FsmAp( ctx );

	/* Make the first state and set it as the start state. */
	StateAp *last = fsm->addState();
	fsm->setStartState( last );

	/* Attach subsequent states. */
	for ( int i = 0; i < len; i++ ) {
		StateAp *newState = fsm->addState();
		fsm->attachNewTrans( last, newState, str[i], str[i] );
		last = newState;
	}

	/* Make the last state the final state. */
	fsm->setFinState( last );

	return fsm;
}

/* Case insensitive version of concatFsm. */
FsmAp *FsmAp::concatFsmCI( FsmCtx *ctx, Key *str, int len )
{
	FsmAp *fsm = new FsmAp( ctx );

	/* Make the first state and set it as the start state. */
	StateAp *last = fsm->addState();
	fsm->setStartState( last );

	/* Attach subsequent states. */
	for ( int i = 0; i < len; i++ ) {
		StateAp *newState = fsm->addState();

		KeySet keySet( ctx->keyOps );
		if ( str[i].isLower() )
			keySet.insert( str[i].toUpper() );
		if ( str[i].isUpper() )
			keySet.insert( str[i].toLower() );
		keySet.insert( str[i] );

		for ( int i = 0; i < keySet.length(); i++ )
			fsm->attachNewTrans( last, newState, keySet[i], keySet[i] );

		last = newState;
	}

	/* Make the last state the final state. */
	fsm->setFinState( last );

	return fsm;
}


/* Construct a machine that matches one character.  A new machine will be made
 * that has two states with a single transition between the states. */
FsmAp *FsmAp::concatFsm( FsmCtx *ctx, Key chr )
{
	FsmAp *fsm = new FsmAp( ctx );

	/* Two states first start, second final. */
	fsm->setStartState( fsm->addState() );

	StateAp *end = fsm->addState();
	fsm->setFinState( end );

	/* Attach on the character. */
	fsm->attachNewTrans( fsm->startState, end, chr, chr );

	return fsm;
}


/* Construct a machine that matches any character in set.  A new machine will
 * be made that has two states and len transitions between the them. The set
 * should be ordered correctly accroding to KeyOps and should not contain
 * any duplicates. */
FsmAp *FsmAp::orFsm( FsmCtx *ctx, Key *set, int len )
{
	FsmAp *fsm = new FsmAp( ctx );

	/* Two states first start, second final. */
	fsm->setStartState( fsm->addState() );

	StateAp *end = fsm->addState();
	fsm->setFinState( end );

	for ( int i = 1; i < len; i++ )
		assert( ctx->keyOps->lt( set[i-1], set[i] ) );

	/* Attach on all the integers in the given string of ints. */
	for ( int i = 0; i < len; i++ )
		fsm->attachNewTrans( fsm->startState, end, set[i], set[i] );

	return fsm;
}

FsmAp *FsmAp::dotFsm( FsmCtx *ctx )
{
	FsmAp *retFsm = FsmAp::rangeFsm( ctx,
			ctx->keyOps->minKey, ctx->keyOps->maxKey );
	return retFsm;
}

FsmAp *FsmAp::dotStarFsm( FsmCtx *ctx )
{
	FsmAp *retFsm = FsmAp::rangeStarFsm( ctx,
			ctx->keyOps->minKey, ctx->keyOps->maxKey );
	return retFsm;
}

/* Construct a machine that matches a range of characters.  A new machine will
 * be made with two states and a range transition between them. The range will
 * match any characters from low to high inclusive. Low should be less than or
 * equal to high otherwise undefined behaviour results.  IsSigned determines
 * if the integers are to be considered as signed or unsigned ints. */
FsmAp *FsmAp::rangeFsm( FsmCtx *ctx, Key low, Key high )
{
	FsmAp *fsm = new FsmAp( ctx );

	/* Two states first start, second final. */
	fsm->setStartState( fsm->addState() );

	StateAp *end = fsm->addState();
	fsm->setFinState( end );

	/* Attach using the range of characters. */
	fsm->attachNewTrans( fsm->startState, end, low, high );

	return fsm;
}

/* Construct a machine that a repeated range of characters.  */
FsmAp *FsmAp::rangeStarFsm( FsmCtx *ctx, Key low, Key high )
{
	FsmAp *fsm = new FsmAp( ctx );

	/* One state which is final and is the start state. */
	fsm->setStartState( fsm->addState() );
	fsm->setFinState( fsm->startState );

	/* Attach start to start using range of characters. */
	fsm->attachNewTrans( fsm->startState, fsm->startState, low, high );

	return fsm;
}

/* Construct a machine that matches the empty string.  A new machine will be
 * made with only one state. The new state will be both a start and final
 * state. IsSigned determines if the machine has a signed or unsigned
 * alphabet. Fsm operations must be done on machines with the same alphabet
 * signedness. */
FsmAp *FsmAp::lambdaFsm( FsmCtx *ctx )
{
	FsmAp *fsm = new FsmAp( ctx );

	/* Give it one state with no transitions making it
	 * the start state and final state. */
	fsm->setStartState( fsm->addState() );
	fsm->setFinState( fsm->startState );

	return fsm;
}

/* Construct a machine that matches nothing at all. A new machine will be
 * made with only one state. It will not be final. */
FsmAp *FsmAp::emptyFsm( FsmCtx *ctx )
{
	FsmAp *fsm = new FsmAp( ctx );

	/* Give it one state with no transitions making it
	 * the start state and final state. */
	fsm->setStartState( fsm->addState() );

	return fsm;
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

/* Fsm concatentation worker. Supports treating the concatentation as optional,
 * which essentially leaves the final states of machine one as final. */
FsmRes FsmAp::doConcat( FsmAp *other, StateSet *fromStates, bool optional )
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
	FsmRes res = fillInStates();
	if ( !res.success() )
		return res;

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );

	return res;
}

FsmRes FsmAp::doUnion( FsmAp *other )
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
	return fillInStates();
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

/* Used near the end of an fsm construction. Any labels that are still around
 * are referenced only by gotos and calls and they need to be made into
 * deterministic entry points. */
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

/* Kleene star operator. Makes this machine the kleene star of itself. Any
 * transitions made going out of the machine and back into itself will be
 * notified that they are leaving transitions by having the leavingFromState
 * callback invoked. */
FsmRes FsmAp::starOp( FsmAp *fsm )
{
	/* The start func orders need to be shifted before doing the star. */
	fsm->ctx->curActionOrd += fsm->shiftStartActionOrder( fsm->ctx->curActionOrd );

	/* Turn on misfit accounting to possibly catch the old start state. */
	fsm->setMisfitAccounting( true );

	/* Create the new new start state. It will be set final after the merging
	 * of the final states with the start state is complete. */
	StateAp *prevStartState = fsm->startState;
	fsm->unsetStartState();
	fsm->setStartState( fsm->addState() );

	/* Merge the new start state with the old one to isolate it. */
	fsm->mergeStates( fsm->startState, prevStartState );

	/* Merge the start state into all final states. Except the start state on
	 * the first pass. If the start state is set final we will be doubling up
	 * its transitions, which will get transfered to any final states that
	 * follow it in the final state set. This will be determined by the order
	 * of items in the final state set. To prevent this we just merge with the
	 * start on a second pass. */
	for ( StateSet::Iter st = fsm->finStateSet; st.lte(); st++ ) {
		if ( *st != fsm->startState )
			fsm->mergeStatesLeaving( *st, fsm->startState );
	}

	/* Now it is safe to merge the start state with itself (provided it
	 * is set final). */
	if ( fsm->startState->isFinState() )
		fsm->mergeStatesLeaving( fsm->startState, fsm->startState );

	/* Now ensure the new start state is a final state. */
	fsm->setFinState( fsm->startState );

	/* Fill in any states that were newed up as combinations of others. */
	FsmRes res = fsm->fillInStates();
	if ( !res.success() )
		return res;

	/* Remove the misfits and turn off misfit accounting. */
	fsm->removeMisfits();
	fsm->setMisfitAccounting( false );

	fsm->afterOpMinimize();

	return res;
}

FsmRes FsmAp::plusOp( FsmAp *fsm )
{
	/* Need a duplicate for the star end. */
	FsmAp *factorDup = new FsmAp( *fsm );

	/* Star the duplicate. */
	FsmRes res1 = FsmAp::starOp( factorDup );
	if ( !res1.success() )
		return res1;

	FsmRes res2 = FsmAp::concatOp( fsm, res1.fsm );
	if ( !res2.success() )
		return res2;

	return res2;
}

FsmRes FsmAp::questionOp( FsmAp *fsm )
{
	/* Make the null fsm. */
	FsmAp *nu = FsmAp::lambdaFsm( fsm->ctx );

	/* Perform the question operator. */
	FsmRes res = FsmAp::unionOp( fsm, nu );
	if ( !res.success() )
		return res;

	return res;
}

FsmRes FsmAp::exactRepeatOp( FsmAp *fsm, int times )
{
	/* Zero repetitions produces lambda machine. */
	if ( times == 0 ) {
		FsmCtx *fsmCtx = fsm->ctx;
		delete fsm;
		return FsmRes( FsmRes::Fsm(), FsmAp::lambdaFsm( fsmCtx ) );
	}

	/* The start func orders need to be shifted before doing the
	 * repetition. */
	fsm->ctx->curActionOrd += fsm->shiftStartActionOrder( fsm->ctx->curActionOrd );

	/* A repeat of one does absolutely nothing. */
	if ( times == 1 )
		return FsmRes( FsmRes::Fsm(), fsm );

	/* Make a machine to make copies from. */
	FsmAp *copyFrom = new FsmAp( *fsm );

	/* Concatentate duplicates onto the end up until before the last. */
	for ( int i = 1; i < times-1; i++ ) {
		FsmAp *dup = new FsmAp( *copyFrom );
		FsmRes res = fsm->doConcat( dup, 0, false );
		if ( !res.success() )
			return res;
	}

	/* Now use the copyFrom on the end. */
	FsmRes res = fsm->doConcat( copyFrom, 0, false );
	if ( !res.success())
		return res;

	res.fsm->afterOpMinimize();

	return res;
}

FsmRes FsmAp::maxRepeatOp( FsmAp *fsm, int times )
{
	/* Zero repetitions produces lambda machine. */
	if ( times == 0 ) {
		FsmCtx *fsmCtx = fsm->ctx;
		delete fsm;
		return FsmRes( FsmRes::Fsm(), FsmAp::lambdaFsm( fsmCtx ) );
	}

	fsm->ctx->curActionOrd += fsm->shiftStartActionOrder( fsm->ctx->curActionOrd );

	/* A repeat of one optional merely allows zero string. */
	if ( times == 1 ) {
		isolateStartState( fsm );
		fsm->setFinState( fsm->startState );
		return FsmRes( FsmRes::Fsm(), fsm );
	}

	/* Make a machine to make copies from. */
	FsmAp *copyFrom = new FsmAp( *fsm );

	/* The state set used in the from end of the concatentation. Starts with
	 * the initial final state set, then after each concatenation, gets set to
	 * the the final states that come from the the duplicate. */
	StateSet lastFinSet( fsm->finStateSet );

	/* Set the initial state to zero to allow zero copies. */
	isolateStartState( fsm );
	fsm->setFinState( fsm->startState );

	/* Concatentate duplicates onto the end up until before the last. */
	for ( int i = 1; i < times-1; i++ ) {
		/* Make a duplicate for concating and set the fin bits to graph 2 so we
		 * can pick out it's final states after the optional style concat. */
		FsmAp *dup = new FsmAp( *copyFrom );
		dup->setFinBits( STB_GRAPH2 );
		FsmRes res = fsm->doConcat( dup, &lastFinSet, true );
		if ( !res.success() )
			return res;

		/* Clear the last final state set and make the new one by taking only
		 * the final states that come from graph 2.*/
		lastFinSet.empty();
		for ( int i = 0; i < fsm->finStateSet.length(); i++ ) {
			/* If the state came from graph 2, add it to the last set and clear
			 * the bits. */
			StateAp *fs = fsm->finStateSet[i];
			if ( fs->stateBits & STB_GRAPH2 ) {
				lastFinSet.insert( fs );
				fs->stateBits &= ~STB_GRAPH2;
			}
		}
	}

	/* Now use the copyFrom on the end, no bits set, no bits to clear. */
	FsmRes res = fsm->doConcat( copyFrom, &lastFinSet, true );

	res.fsm->afterOpMinimize();

	return res;
}

FsmRes FsmAp::minRepeatOp( FsmAp *fsm, int times )
{
	if ( times == 0 ) {
		/* Acts just like a star op on the machine to return. */
		FsmRes res = FsmAp::starOp( fsm );
		if ( !res.success() )
			return res;

		return res;
	}
	else {
		/* Take a duplicate for the plus. */
		FsmAp *dup = new FsmAp( *fsm );

		/* Do repetition on the first half. */
		FsmRes res1 = FsmAp::exactRepeatOp( fsm, times );
		if ( !res1.success() )
			return res1;

		/* Star the duplicate. */
		FsmRes res2 = FsmAp::starOp( dup );
		if ( !res2.success() )
			return res2;

		/* Tack on the kleene star. */
		FsmRes res3 = FsmAp::concatOp( res1.fsm, res2.fsm );
		if ( !res3.success() )
			return res3;

		return res3;
	}
}

FsmRes FsmAp::rangeRepeatOp( FsmAp *fsm, int lowerRep, int upperRep )
{
	if ( lowerRep == 0 && upperRep == 0 ) {
		FsmCtx *fsmCtx = fsm->ctx;
		delete fsm;
		return FsmRes( FsmRes::Fsm(), FsmAp::lambdaFsm( fsmCtx ) );
	}
	else if ( lowerRep == 0 ) {
		/* Just doing max repetition. Already guarded against n == 0. */
		FsmRes res = FsmAp::maxRepeatOp( fsm, upperRep );
		if ( !res.success() )
			return res;

		return res;
	}
	else if ( lowerRep == upperRep ) {
		/* Just doing exact repetition. Already guarded against n == 0. */
		FsmRes res = FsmAp::exactRepeatOp( fsm, lowerRep );
		if ( !res.success() )
			return res;

		return res;
	}
	else {
		/* This is the case that 0 < lowerRep < upperRep. Take a
		 * duplicate for the optional repeat. */
		FsmAp *dup = new FsmAp( *fsm );

		/* Do repetition on the first half. */
		FsmRes res1 = FsmAp::exactRepeatOp( fsm, lowerRep );
		if ( !res1.success() )
			return res1;

		/* Do optional repetition on the second half. */
		FsmRes res2 = FsmAp::maxRepeatOp( dup, upperRep - lowerRep );
		if ( !res2.success() )
			return res2;

		/* Tak on the duplicate machine. */
		FsmRes res3 = FsmAp::concatOp( res1.fsm, res2.fsm );
		if ( !res3.success() )
			return res3;

		return res3;
	}
}

/* Concatenates other to the end of this machine. Other is deleted.  Any
 * transitions made leaving this machine and entering into other are notified
 * that they are leaving transitions by having the leavingFromState callback
 * invoked. */
FsmRes FsmAp::concatOp( FsmAp *fsm, FsmAp *other, bool lastInSeq )
{
	for ( PriorTable::Iter g = other->startState->guardedInTable; g.lte(); g++ ) {
		fsm->allTransPrior( 0, g->desc );
		other->allTransPrior( 0, g->desc->other );
	}

	/* Assert same signedness and return graph concatenation op. */
	assert( fsm->ctx == other->ctx );

	FsmRes res = fsm->doConcat( other, 0, false );
	if ( !res.success() ) {
		delete fsm;
		return res;
	}

	res.fsm->afterOpMinimize( lastInSeq );

	return res;
}

FsmRes FsmAp::rightStartConcatOp( FsmAp *fsm, FsmAp *other, bool lastInSeq )
{
	PriorDesc *priorDesc0 = fsm->ctx->allocPriorDesc();
	PriorDesc *priorDesc1 = fsm->ctx->allocPriorDesc();

	/* Set up the priority descriptors. The left machine gets the
	 * lower priority where as the right get the higher start priority. */
	priorDesc0->key = fsm->ctx->nextPriorKey++;
	priorDesc0->priority = 0;
	fsm->allTransPrior( fsm->ctx->curPriorOrd++, priorDesc0 );

	/* The start transitions of the right machine gets the higher
	 * priority. Use the same unique key. */
	priorDesc1->key = priorDesc0->key;
	priorDesc1->priority = 1;
	other->startFsmPrior( fsm->ctx->curPriorOrd++, priorDesc1 );

	return concatOp( fsm, other, lastInSeq );
}

/* Returns union of fsm and other. Other is deleted. */
FsmRes FsmAp::unionOp( FsmAp *fsm, FsmAp *other, bool lastInSeq )
{
	assert( fsm->ctx == other->ctx );

	fsm->ctx->unionOp = true;

	fsm->setFinBits( STB_GRAPH1 );
	other->setFinBits( STB_GRAPH2 );

	/* Turn on misfit accounting for both graphs. */
	fsm->setMisfitAccounting( true );
	other->setMisfitAccounting( true );

	/* Call Worker routine. */
	FsmRes res = fsm->doUnion( other );
	if ( !res.success() )
		return res;

	/* Remove the misfits and turn off misfit accounting. */
	fsm->removeMisfits();
	fsm->setMisfitAccounting( false );

	fsm->ctx->unionOp = false;
	fsm->unsetFinBits( STB_BOTH );

	fsm->afterOpMinimize( lastInSeq );

	return res;
}

/* Intersects other with this machine. Other is deleted. */
FsmRes FsmAp::intersectOp( FsmAp *fsm, FsmAp *other, bool lastInSeq )
{
	assert( fsm->ctx == other->ctx );

	/* Turn on misfit accounting for both graphs. */
	fsm->setMisfitAccounting( true );
	other->setMisfitAccounting( true );

	/* Set the fin bits on this and other to want each other. */
	fsm->setFinBits( STB_GRAPH1 );
	other->setFinBits( STB_GRAPH2 );

	/* Call worker Or routine. */
	FsmRes res = fsm->doUnion( other );
	if ( !res.success() )
		return res;

	/* Unset any final states that are no longer to 
	 * be final due to final bits. */
	fsm->unsetIncompleteFinals();

	/* Remove the misfits and turn off misfit accounting. */
	fsm->removeMisfits();
	fsm->setMisfitAccounting( false );

	/* Remove states that have no path to a final state. */
	fsm->removeDeadEndStates();

	fsm->afterOpMinimize( lastInSeq );

	return res;
}

/* Set subtracts other machine from this machine. Other is deleted. */
FsmRes FsmAp::subtractOp( FsmAp *fsm, FsmAp *other, bool lastInSeq )
{
	assert( fsm->ctx == other->ctx );

	/* Turn on misfit accounting for both graphs. */
	fsm->setMisfitAccounting( true );
	other->setMisfitAccounting( true );

	/* Set the fin bits of other to be killers. */
	other->setFinBits( STB_GRAPH1 );

	/* Call worker Or routine. */
	FsmRes res = fsm->doUnion( other );
	if ( !res.success() )
		return res;

	/* Unset any final states that are no longer to 
	 * be final due to final bits. */
	fsm->unsetKilledFinals();

	/* Remove the misfits and turn off misfit accounting. */
	fsm->removeMisfits();
	fsm->setMisfitAccounting( false );

	/* Remove states that have no path to a final state. */
	fsm->removeDeadEndStates();

	fsm->afterOpMinimize( lastInSeq );

	return res;
}

FsmRes FsmAp::epsilonOp( FsmAp *fsm )
{
	fsm->setMisfitAccounting( true );

	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ )
		st->owningGraph = 0;

	/* Perform merges. */
	fsm->resolveEpsilonTrans();

	/* Epsilons can caused merges which leave behind unreachable states. */
	FsmRes res = fsm->fillInStates();
	if ( !res.success() )
		return res;

	/* Remove the misfits and turn off misfit accounting. */
	fsm->removeMisfits();
	fsm->setMisfitAccounting( false );

	return res;
}

/* Make a new maching by joining together a bunch of machines without making
 * any transitions between them. A negative finalId results in there being no
 * final id. */
FsmRes FsmAp::joinOp( FsmAp *fsm, int startId, int finalId, FsmAp **others, int numOthers )
{
	for ( int m = 0; m < numOthers; m++ ) {
		assert( fsm->ctx == others[m]->ctx );
	}

	/* Set the owning machines. Start at one. Zero is reserved for the start
	 * and final states. */
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ )
		st->owningGraph = 1;
	for ( int m = 0; m < numOthers; m++ ) {
		for ( StateList::Iter st = others[m]->stateList; st.lte(); st++ )
			st->owningGraph = 2+m;
	}

	/* All machines loose start state status. */
	fsm->unsetStartState();
	for ( int m = 0; m < numOthers; m++ )
		others[m]->unsetStartState();
	
	/* Bring the other machines into this. */
	for ( int m = 0; m < numOthers; m++ ) {
		/* Bring in the rest of other's entry points. */
		fsm->copyInEntryPoints( others[m] );
		others[m]->entryPoints.empty();

		/* Merge the lists. This will move all the states from other into
		 * this. No states will be deleted. */
		fsm->stateList.append( others[m]->stateList );
		assert( others[m]->misfitList.length() == 0 );

		/* Move the final set data from other into this. */
		fsm->finStateSet.insert( others[m]->finStateSet );
		others[m]->finStateSet.empty();

		/* Since other's list is empty, we can delete the fsm without
		 * affecting any states. */
		delete others[m];
	}

	/* Look up the start entry point. */
	EntryMapEl *enLow = 0, *enHigh = 0;
	bool findRes = fsm->entryPoints.findMulti( startId, enLow, enHigh );
	if ( ! findRes ) {
		/* No start state. Set a default one and proceed with the join. Note
		 * that the result of the join will be a very uninteresting machine. */
		fsm->setStartState( fsm->addState() );
	}
	else {
		/* There is at least one start state, create a state that will become
		 * the new start state. */
		StateAp *newStart = fsm->addState();
		fsm->setStartState( newStart );

		/* The start state is in an owning machine class all it's own. */
		newStart->owningGraph = 0;

		/* Create the set of states to merge from. */
		StateSet stateSet;
		for ( EntryMapEl *en = enLow; en <= enHigh; en++ )
			stateSet.insert( en->value );

		/* Merge in the set of start states into the new start state. */
		fsm->mergeStateList( newStart, stateSet.data, stateSet.length() );
	}

	/* Take a copy of the final state set, before unsetting them all. This
	 * will allow us to call clearOutData on the states that don't get
	 * final state status back back. */
	StateSet finStateSetCopy = fsm->finStateSet;

	/* Now all final states are unset. */
	fsm->unsetAllFinStates();

	if ( finalId >= 0 ) {
		/* Create the implicit final state. */
		StateAp *finState = fsm->addState();
		fsm->setFinState( finState );

		/* Assign an entry into the final state on the final state entry id. Note
		 * that there may already be an entry on this id. That's ok. Also set the
		 * final state owning machine id. It's in a class all it's own. */
		fsm->setEntry( finalId, finState );
		finState->owningGraph = 0;
	}

	/* Hand over to workers for resolving epsilon trans. This will merge states
	 * with the targets of their epsilon transitions. */
	fsm->resolveEpsilonTrans();

	/* Invoke the relinquish final callback on any states that did not get
	 * final state status back. */
	for ( StateSet::Iter st = finStateSetCopy; st.lte(); st++ ) {
		if ( !((*st)->stateBits & STB_ISFINAL) )
			fsm->clearOutData( *st );
	}

	/* Fill in any new states made from merging. */
	FsmRes res = fsm->fillInStates();
	if ( !res.success() )
		return res;

	/* Joining can be messy. Instead of having misfit accounting on (which is
	 * tricky here) do a full cleaning. */
	fsm->removeUnreachableStates();

	return res;
}

/* Ensure that the start state is free of entry points (aside from the fact
 * that it is the start state). If the start state has entry points then Make a
 * new start state by merging with the old one. Useful before modifying start
 * transitions. If the existing start state has any entry points other than the
 * start state entry then modifying its transitions changes more than the start
 * transitions. So isolate the start state by separating it out such that it
 * only has start stateness as it's entry point. */
FsmRes FsmAp::isolateStartState( FsmAp *fsm )
{
	/* Do nothing if the start state is already isolated. */
	if ( fsm->isStartStateIsolated() )
		return FsmRes( FsmRes::Fsm(), fsm );

	/* Turn on misfit accounting to possibly catch the old start state. */
	fsm->setMisfitAccounting( true );

	/* This will be the new start state. The existing start
	 * state is merged with it. */
	StateAp *prevStartState = fsm->startState;
	fsm->unsetStartState();
	fsm->setStartState( fsm->addState() );

	/* Merge the new start state with the old one to isolate it. */
	fsm->mergeStates( fsm->startState, prevStartState );

	/* Stfil and stateDict will be empty because the merging of the old start
	 * state into the new one will not have any conflicting transitions. */
	assert( fsm->stateDict.treeSize == 0 );
	assert( fsm->nfaList.length() == 0 );

	/* The old start state may be unreachable. Remove the misfits and turn off
	 * misfit accounting. */
	fsm->removeMisfits();
	fsm->setMisfitAccounting( false );

	return FsmRes( FsmRes::Fsm(), fsm );
}

StateAp *FsmAp::dupStartState()
{
	StateAp *dup = addState();
	mergeStates( dup, startState );
	return dup;
}

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
			/* Looking for unequal guarded priorities with the same key. */
			if ( pd1->desc->key == pd2->desc->key ) {
				if ( pd1->desc->priority < pd2->desc->priority || 
						pd1->desc->priority > pd2->desc->priority )
				{
					if ( ctx->nfaTermCheck && pd1->desc->guarded ) {
						if ( ! priorInteraction ) {
							priorInteraction = true;
							guardId = pd1->desc->guardId;
						}
					}
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
			
void FsmAp::cleanAbortedFill()
{
	while ( nfaList.length() > 0 ) {
		StateAp *state = nfaList.head;

		StateSet *stateSet = &state->stateDictEl->stateSet;
		//mergeStateList( state, stateSet->data, stateSet->length() );

		for ( StateSet::Iter s = *stateSet; s.lte(); s++ )
			detachStateDict( state, *s );

		nfaList.detach( state );
	}

	/* Disassociated state dict elements from states. */
	for ( StateDict::Iter sdi = stateDict; sdi.lte(); sdi++ )
		sdi->targState->stateDictEl = 0;

	/* Delete all the state dict elements. */
	stateDict.empty();

	/* Delete all the transitions. */
	for ( StateList::Iter state = stateList; state.lte(); state++ ) {
		/* Iterate the out transitions, deleting them. */
		for ( TransList::Iter n, t = state->outList; t.lte(); ) {
			n = t.next();
			if ( t->plain() )
				delete t->tdap();
			else
				delete t->tcap();
			t = n;
		}
		state->outList.abandon();
	}

	/* Delete all the states. */
	stateList.empty();

	/* Delete all the transitions. */
	for ( StateList::Iter state = misfitList; state.lte(); state++ ) {
		/* Iterate the out transitions, deleting them. */
		for ( TransList::Iter n, t = state->outList; t.lte(); ) {
			n = t.next();
			if ( t->plain() )
				delete t->tdap();
			else
				delete t->tcap();
			t = n;
		}
		state->outList.abandon();
	}

	/* Delete all the states. */
	misfitList.empty();
}

bool FsmAp::overStateLimit()
{
	if ( ctx->stateLimit > 0 ) {
		long states = misfitList.length() + stateList.length();
		if ( states > ctx->stateLimit )
			return true;
	}
	return false;
}

FsmRes FsmAp::fillInStates()
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

		if ( priorInteraction ) {
			cleanAbortedFill();
			return FsmRes( FsmRes::PriorInteraction(), guardId );
		}

		if ( overStateLimit() ) {
			cleanAbortedFill();
			return FsmRes( FsmRes::TooManyStates() );
		}
	}

	if ( priorInteraction ) {
		cleanAbortedFill();
		return FsmRes( FsmRes::PriorInteraction(), guardId );
	}

	if ( overStateLimit() ) {
		cleanAbortedFill();
		return FsmRes( FsmRes::TooManyStates() );
	}

	/* The NFA list is empty at this point. There are no state sets we need to
	 * preserve. */

	/* Disassociated state dict elements from states. */
	for ( StateDict::Iter sdi = stateDict; sdi.lte(); sdi++ )
		sdi->targState->stateDictEl = 0;

	/* Delete all the state dict elements. */
	stateDict.empty();

	return FsmRes( FsmRes::Fsm(), this );
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

FsmRes FsmAp::condCostFromState( FsmAp *fsm, StateAp *state, long depth )
{
	/* Nothing to do if the state is already on the list. */
	if ( state->stateBits & STB_ONLIST )
		return FsmRes( FsmRes::Fsm(), fsm );

	if ( depth > fsm->ctx->nfaCondsDepth )
		return FsmRes( FsmRes::Fsm(), fsm );

	/* Doing depth first, put state on the list. */
	state->stateBits |= STB_ONLIST;

	/* Recurse on everything ranges. */
	for ( TransList::Iter trans = state->outList; trans.lte(); trans++ ) {
		if ( trans->plain() ) {
			if ( trans->tdap()->toState != 0 ) {
				FsmRes res = condCostFromState( fsm, trans->tdap()->toState, depth + 1 );
				if ( !res.success() )
					return res;
			}
		}
		else {
			for ( CondSet::Iter csi = trans->condSpace->condSet; csi.lte(); csi++ ) {
				if ( (*csi)->costMark )
					return FsmRes( FsmRes::CondCostTooHigh(), (*csi)->costId );
			}
			
			for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) {
				if ( cond->toState != 0 ) {
					FsmRes res = condCostFromState( fsm, cond->toState, depth + 1 );
					if ( !res.success() )
						return res;
				}
			}
		}
	}

	if ( state->nfaOut != 0 ) {
		for ( NfaTransList::Iter n = *state->nfaOut; n.lte(); n++ ) {
			/* We do not increment depth here since this is an epsilon transition. */
			FsmRes res = condCostFromState( fsm, n->toState, depth );
			if ( !res.success() )
				return res;
		}
	}

	for ( ActionTable::Iter a = state->fromStateActionTable; a.lte(); a++ ) {
		if ( a->value->costMark )
			return FsmRes( FsmRes::CondCostTooHigh(), a->value->costId );
	}

	return FsmRes( FsmRes::Fsm(), fsm );
}


/* Returns either success (using supplied fsm, or some error condition. Does
 * not delete the fsm (under any condition). */
FsmRes FsmAp::condCostSearch( FsmAp *fsmAp )
{
	/* Init on state list flags. */
	for ( StateList::Iter st = fsmAp->stateList; st.lte(); st++ )
		st->stateBits &= ~STB_ONLIST;

	return condCostFromState( fsmAp, fsmAp->startState, 1 );
}

void FsmAp::condCost( Action *action, long repId )
{
	action->costMark = true;
	action->costId = repId;
}


void FsmAp::applyEntryPriorGuard( FsmAp *fsm, long repId )
{
	PriorDesc *priorDesc0 = fsm->ctx->allocPriorDesc();
	PriorDesc *priorDesc1 = fsm->ctx->allocPriorDesc();

	priorDesc0->key = fsm->ctx->nextPriorKey;
	priorDesc0->priority = 0;
	priorDesc0->guarded = true;
	priorDesc0->guardId = repId;
	priorDesc0->other = priorDesc1;

	priorDesc1->key = fsm->ctx->nextPriorKey;
	priorDesc1->priority = 1;
	priorDesc1->guarded = true;
	priorDesc1->guardId = repId;
	priorDesc1->other = priorDesc0;

	/* Roll over for next allocation. */
	fsm->ctx->nextPriorKey += 1;

	/* Only need to set the first. Second is referenced using 'other' field. */
	fsm->startState->guardedInTable.setPrior( 0, priorDesc0 );
}

void FsmAp::applyRepeatPriorGuard( FsmAp *fsm, long repId )
{
	PriorDesc *priorDesc2 = fsm->ctx->allocPriorDesc();
	PriorDesc *priorDesc3 = fsm->ctx->allocPriorDesc();

	priorDesc2->key = fsm->ctx->nextPriorKey;
	priorDesc2->priority = 0;
	priorDesc2->guarded = true;
	priorDesc2->guardId = repId;
	priorDesc2->other = priorDesc3;

	priorDesc3->key = fsm->ctx->nextPriorKey;
	priorDesc3->guarded = true;
	priorDesc3->priority = 1;
	priorDesc3->guardId = repId;
	priorDesc3->other = priorDesc2;

	/* Roll over for next allocation. */
	fsm->ctx->nextPriorKey += 1;

	/* Only need to set the first. Second is referenced using 'other' field. */
	fsm->startState->guardedInTable.setPrior( 0, priorDesc2 );
	
	fsm->allTransPrior( fsm->ctx->curPriorOrd++, priorDesc3 );
	fsm->leaveFsmPrior( fsm->ctx->curPriorOrd++, priorDesc2 );
}

FsmRes FsmAp::condPlus( FsmAp *fsm, long repId, Action *ini, Action *inc, Action *min, Action *max )
{
	condCost( ini, repId );
	condCost( inc, repId );
	condCost( min, repId );
	if ( max != 0 )
		condCost( max, repId );

	fsm->startFsmAction( 0, inc );

	if ( max != 0 ) {
		FsmRes res = fsm->startFsmCondition( max, true );
		if ( !res.success() )
			return res;
	}

	/* Need a duplicated for the star end. */
	FsmAp *dup = new FsmAp( *fsm );

	applyRepeatPriorGuard( dup, repId );

	/* Star the duplicate. */
	FsmRes res1 = FsmAp::starOp( dup );
	if ( !res1.success() )
		return res1;

	FsmRes res2 = FsmAp::concatOp( fsm, res1.fsm );
	if ( !res2.success() )
		return res2;

	/* End plus operation. */

	res2.fsm->leaveFsmCondition( min, true );

	/* Init action. */
	res2.fsm->startFromStateAction( 0,  ini );

	/* Leading priority guard. */
	applyEntryPriorGuard( res2.fsm, repId );

	return res2;
}

FsmRes FsmAp::condStar( FsmAp *fsm, long repId, Action *ini, Action *inc, Action *min, Action *max )
{
	FsmRes cp = condPlus( fsm, repId, ini, inc, min, max );
	if ( !cp.success() )
		return cp;

	StateAp *newStart = cp.fsm->dupStartState();
	cp.fsm->unsetStartState();
	cp.fsm->setStartState( newStart );

	/* Now ensure the new start state is a final state. */
	cp.fsm->setFinState( newStart );
	cp.fsm->addOutCondition( newStart, min, true );

	return cp;
}

/* Remove duplicates of unique actions from an action table. */
void FsmAp::removeDups( ActionTable &table )
{
	/* Scan through the table looking for unique actions to 
	 * remove duplicates of. */
	for ( int i = 0; i < table.length(); i++ ) {
		/* Remove any duplicates ahead of i. */
		for ( int r = i+1; r < table.length(); ) {
			if ( table[r].value == table[i].value )
				table.vremove(r);
			else
				r += 1;
		}
	}
}

/* Remove duplicates from action lists. This operates only on transition and
 * eof action lists and so should be called once all actions have been
 * transfered to their final resting place. */
void FsmAp::removeActionDups()
{
	/* Loop all states. */
	for ( StateList::Iter state = stateList; state.lte(); state++ ) {
		/* Loop all transitions. */
		for ( TransList::Iter trans = state->outList; trans.lte(); trans++ ) {
			if ( trans->plain() )
				removeDups( trans->tdap()->actionTable );
			else {
				for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ )
					removeDups( cond->actionTable );
			}
		}
		removeDups( state->toStateActionTable );
		removeDups( state->fromStateActionTable );
		removeDups( state->eofActionTable );
	}
}

