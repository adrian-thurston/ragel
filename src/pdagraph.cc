/*
 * Copyright 2006-2012 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pdagraph.h"

#include <assert.h>
#include <stdbool.h>

#include <iostream>

using std::cerr;
using std::endl;

/* Create a new fsm state. State has not out transitions or in transitions, not
 * out out transition data and not number. */
PdaState::PdaState()
:
	/* No in transitions. */
	inRange(),

	/* No entry points, or epsilon trans. */
	pendingCommits(),

	stateSet(0),

	/* Only used during merging. Normally null. */
	stateDictEl(0),

	/* No state identification bits. */
	stateBits(0),

	onClosureQueue(false),
	inClosedMap(false),
	followMarked(false),

	advanceReductions(false)
{
}

/* Copy everything except the action transitions. That is left up to the
 * PdaGraph copy constructor. */
PdaState::PdaState(const PdaState &other)
:
	inRange(),

	/* Duplicate the entry id set, epsilon transitions and context sets. These
	 * are sets of integers and as such need no fixing. */
	pendingCommits(other.pendingCommits),

	stateSet(0),

	/* This is only used during merging. Normally null. */
	stateDictEl(0),

	/* Fsm state data. */
	stateBits(other.stateBits),

	dotSet(other.dotSet),
	onClosureQueue(false),
	inClosedMap(false),
	followMarked(false),

	transMap()
{
	/* Duplicate all the transitions. */
	for ( TransMap::Iter trans = other.transMap; trans.lte(); trans++ ) {
		/* Dupicate and store the orginal target in the transition. This will
		 * be corrected once all the states have been created. */
		PdaTrans *newTrans = new PdaTrans(*trans->value);
		newTrans->toState = trans->value->toState;
		transMap.append( TransMapEl( newTrans->lowKey, newTrans ) );
	}
}

/* If there is a state dict element, then delete it. Everything else is left
 * up to the FsmGraph destructor. */
PdaState::~PdaState()
{
	if ( stateDictEl != 0 )
		delete stateDictEl;
}

/* Graph constructor. */
PdaGraph::PdaGraph()
:
	/* No start state. */
	startState(0)
{
}

/* Copy all graph data including transitions. */
PdaGraph::PdaGraph( const PdaGraph &graph )
:
	/* Lists start empty. Will be filled by copy. */
	stateList(),
	misfitList(),

	/* Copy in the entry points, 
	 * pointers will be resolved later. */
	startState(graph.startState),

	/* Will be filled by copy. */
	finStateSet()
{
	/* Create the states and record their map in the original state. */
	PdaStateList::Iter origState = graph.stateList;
	for ( ; origState.lte(); origState++ ) {
		/* Make the new state. */
		PdaState *newState = new PdaState( *origState );

		/* Add the state to the list.  */
		stateList.append( newState );

		/* Set the mapsTo item of the old state. */
		origState->stateMap = newState;
	}
	
	/* Derefernce all the state maps. */
	for ( PdaStateList::Iter state = stateList; state.lte(); state++ ) {
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			/* The points to the original in the src machine. The taget's duplicate
			 * is in the statemap. */
			PdaState *toState = trans->value->toState != 0 ? 
					trans->value->toState->stateMap : 0;

			/* Attach The transition to the duplicate. */
			trans->value->toState = 0;
			attachTrans( state, toState, trans->value );
		}
	}

	/* Fix the start state pointer and the new start state's count of in
	 * transiions. */
	startState = startState->stateMap;

	/* Build the final state set. */
	PdaStateSet::Iter st = graph.finStateSet; 
	for ( ; st.lte(); st++ ) 
		finStateSet.insert((*st)->stateMap);
}

/* Deletes all transition data then deletes each state. */
PdaGraph::~PdaGraph()
{
	/* Delete all the transitions. */
	PdaStateList::Iter state = stateList;
	for ( ; state.lte(); state++ ) {
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ )
			delete trans->value;
	}

	/* Delete all the states. */
	stateList.empty();
}

/* Set a state final. The state has its isFinState set to true and the state
 * is added to the finStateSet. */
void PdaGraph::setFinState( PdaState *state )
{
	/* Is it already a fin state. */
	if ( state->stateBits & SB_ISFINAL )
		return;
	
	state->stateBits |= SB_ISFINAL;
	finStateSet.insert( state );
}

void PdaGraph::unsetAllFinStates( )
{
	for ( PdaStateSet::Iter st = finStateSet; st.lte(); st++ ) {
		PdaState *state = *st;
		state->stateBits &= ~ SB_ISFINAL;
	}
	finStateSet.empty();
}

/* Set and unset a state as the start state. */
void PdaGraph::setStartState( PdaState *state )
{
	/* Sould change from unset to set. */
	assert( startState == 0 );
	startState = state;
}

/* Mark all states reachable from state. Traverses transitions forward. Used
 * for removing states that have no path into them. */
void PdaGraph::markReachableFromHere( PdaState *state )
{
	/* Base case: return; */
	if ( state->stateBits & SB_ISMARKED )
		return;
	
	/* Set this state as processed. We are going to visit all states that this
	 * state has a transition to. */
	state->stateBits |= SB_ISMARKED;

	/* Recurse on all out transitions. */
	for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
		if ( trans->value->toState != 0 )
			markReachableFromHere( trans->value->toState );
	}
}

void PdaGraph::setStateNumbers()
{
	int curNum = 0;
	PdaStateList::Iter state = stateList;
	for ( ; state.lte(); state++ )
		state->stateNum = curNum++;
}

/* Insert a transition into an inlist. The head must be supplied. */
void PdaGraph::attachToInList( PdaState *from, PdaState *to, 
		PdaTrans *&head, PdaTrans *trans )
{
	trans->ilnext = head;
	trans->ilprev = 0;

	/* If in trans list is not empty, set the head->prev to trans. */
	if ( head != 0 )
		head->ilprev = trans;

	/* Now insert ourselves at the front of the list. */
	head = trans;
};

/* Detach a transition from an inlist. The head of the inlist must be supplied. */
void PdaGraph::detachFromInList( PdaState *from, PdaState *to, 
		PdaTrans *&head, PdaTrans *trans )
{
	/* Detach in the inTransList. */
	if ( trans->ilprev == 0 ) 
		head = trans->ilnext; 
	else
		trans->ilprev->ilnext = trans->ilnext; 

	if ( trans->ilnext != 0 )
		trans->ilnext->ilprev = trans->ilprev; 
}

/* Attach states on the default transition, range list or on out/in list key.
 * Type of attaching and is controlled by keyType. First makes a new
 * transition. If there is already a transition out from fromState on the
 * default, then will assertion fail. */
PdaTrans *PdaGraph::appendNewTrans( PdaState *from, PdaState *to, long lowKey, long )
{
	/* Make the new transition. */
	PdaTrans *retVal = new PdaTrans();

	/* The transition is now attached. Remember the parties involved. */
	retVal->fromState = from;
	retVal->toState = to;

	/* Make the entry in the out list for the transitions. */
	from->transMap.append( TransMapEl( lowKey, retVal ) );

	/* Set the the keys of the new trans. */
	retVal->lowKey = lowKey;

	/* Attach using inRange as the head pointer. */
	attachToInList( from, to, to->inRange.head, retVal );

	return retVal;
}

PdaTrans *PdaGraph::insertNewTrans( PdaState *from, PdaState *to, long lowKey, long )
{
	/* Make the new transition. */
	PdaTrans *retVal = new PdaTrans();

	/* The transition is now attached. Remember the parties involved. */
	retVal->fromState = from;
	retVal->toState = to;

	/* Make the entry in the out list for the transitions. */
	from->transMap.insert( lowKey, retVal );

	/* Set the the keys of the new trans. */
	retVal->lowKey = lowKey;

	/* Attach using inRange as the head pointer. */
	attachToInList( from, to, to->inRange.head, retVal );

	return retVal;
}

/* Attach for range lists or for the default transition. Type of attaching is
 * controlled by the keyType parameter. This attach should be used when a
 * transition already is allocated and must be attached to a target state.
 * Does not handle adding the transition into the out list. */
void PdaGraph::attachTrans( PdaState *from, PdaState *to, PdaTrans *trans )
{
	assert( trans->fromState == 0 && trans->toState == 0 );
	trans->fromState = from;
	trans->toState = to;

	/* Attach using the inRange pointer as the head pointer. */
	attachToInList( from, to, to->inRange.head, trans );
}

/* Detach for out/in lists or for default transition. The type of detaching is
 * controlled by the keyType parameter. */
void PdaGraph::detachTrans( PdaState *from, PdaState *to, PdaTrans *trans )
{
	assert( trans->fromState == from && trans->toState == to );
	trans->fromState = 0;
	trans->toState = 0;

	/* Detach using to's inRange pointer as the head. */
	detachFromInList( from, to, to->inRange.head, trans );
}


/* Detach a state from the graph. Detaches and deletes transitions in and out
 * of the state. Empties inList and outList. Removes the state from the final
 * state set. A detached state becomes useless and should be deleted. */
void PdaGraph::detachState( PdaState *state )
{
	/* Detach the in transitions from the inRange list of transitions. */
	while ( state->inRange.head != 0 ) {
		/* Get pointers to the trans and the state. */
		PdaTrans *trans = state->inRange.head;
		PdaState *fromState = trans->fromState;

		/* Detach the transitions from the source state. */
		detachTrans( fromState, state, trans );

		/* Ok to delete the transition. */
		fromState->transMap.remove( trans->lowKey );
		delete trans;
	}

	/* Detach out range transitions. */
	for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
		detachTrans( state, trans->value->toState, trans->value );
		delete trans->value;
	}

	/* Delete all of the out range pointers. */
	state->transMap.empty();

	/* Unset final stateness before detaching from graph. */
	if ( state->stateBits & SB_ISFINAL )
		finStateSet.remove( state );
}

/* Move all the transitions that go into src so that they go into dest.  */
void PdaGraph::inTransMove( PdaState *dest, PdaState *src )
{
	/* Do not try to move in trans to and from the same state. */
	assert( dest != src );

	/* If src is the start state, dest becomes the start state. */
	assert( src != startState );

	/* Move the transitions in inRange. */
	while ( src->inRange.head != 0 ) {
		/* Get trans and from state. */
		PdaTrans *trans = src->inRange.head;
		PdaState *fromState = trans->fromState;

		/* Detach from src, reattach to dest. */
		detachTrans( fromState, src, trans );
		attachTrans( fromState, dest, trans );
	}
}

void PdaGraph::addInReduction( PdaTrans *dest, long prodId, long prior )
{
	/* Look for the reduction. If not there insert it, otherwise take
	 * the max of the priorities. */
	ReductionMapEl *redMapEl = dest->reductions.find( prodId );
	if ( redMapEl == 0 )
		dest->reductions.insert( prodId, prior );
	else if ( prior > redMapEl->value )
		redMapEl->value = prior;
}

/* Callback invoked when another trans (or possibly this) is added into this
 * transition during the merging process.  Draw in any properties of srcTrans
 * into this transition. AddInTrans is called when a new transitions is made
 * that will be a duplicate of another transition or a combination of several
 * other transitions. AddInTrans will be called for each transition that the
 * new transition is to represent. */
void PdaGraph::addInTrans( PdaTrans *destTrans, PdaTrans *srcTrans )
{
	/* Protect against adding in from ourselves. */
	if ( srcTrans != destTrans ) {

		/* Add in the shift priority. */
		if ( destTrans->isShift && srcTrans->isShift ) {
			/* Both shifts are set. We want the max of the two. */
			if ( srcTrans->shiftPrior > destTrans->shiftPrior )
				destTrans->shiftPrior = srcTrans->shiftPrior;
		}
		else if ( srcTrans->isShift ) {
			/* Just the source is set, copy the source prior over. */
			destTrans->shiftPrior = srcTrans->shiftPrior;
		}

		/* If either is a shift, dest is a shift. */
		destTrans->isShift = destTrans->isShift || srcTrans->isShift;

		/* Add in the reductions. */
		for ( ReductionMap::Iter red = srcTrans->reductions; red.lte(); red++ )
			addInReduction( destTrans, red->key, red->value );

		/* Add in the commit points. */
		destTrans->commits.insert( srcTrans->commits );

		if ( srcTrans->toState->advanceReductions )
			destTrans->toState->advanceReductions = true;

		if ( srcTrans->noPreIgnore )
			destTrans->noPreIgnore = true;
		if ( srcTrans->noPostIgnore )
			destTrans->noPostIgnore = true;
	}
}

/* NO LONGER USED. */
void PdaGraph::addInState( PdaState *destState, PdaState *srcState )
{
	/* Draw in any properties of srcState into destState. */
	if ( srcState != destState ) {
		/* Get the epsilons, context, out priorities. */
		destState->pendingCommits.insert( srcState->pendingCommits );
		if ( srcState->pendingCommits.length() > 0 )
			cerr << "THERE ARE PENDING COMMITS DRAWN IN" << endl;

		/* Parser generation data. */
		destState->dotSet.insert( srcState->dotSet );

		if ( srcState->onClosureQueue && !destState->onClosureQueue ) {
			stateClosureQueue.append( destState );
			destState->onClosureQueue = true;
		}
	}
}

/* Make a new state. The new state will be put on the graph's
 * list of state. The new state can be created final or non final. */
PdaState *PdaGraph::addState()
{
	/* Make the new state to return. */
	PdaState *state = new PdaState();

	/* Create the new state. */
	stateList.append( state );

	return state;
}


/* Follow from to the final state of srcFsm. */
PdaState *PdaGraph::followFsm( PdaState *from, PdaGraph *srcFsm )
{
	PdaState *followSrc = srcFsm->startState;

	while ( ! followSrc->isFinState() ) {
		assert( followSrc->transMap.length() == 1 );
		PdaTrans *followTrans = followSrc->transMap[0].value;

		PdaTrans *inTrans = from->findTrans( followTrans->lowKey );
		assert( inTrans != 0 );

		from = inTrans->toState;
		followSrc = followTrans->toState;
	}

	return from;
}

int PdaGraph::fsmLength( )
{
	int length = 0;
	PdaState *state = startState;
	while ( ! state->isFinState() ) {
		length += 1;
		state = state->transMap[0].value->toState;
	}
	return length;
}

/* Remove states that have no path to them from the start state. Recursively
 * traverses the graph marking states that have paths into them. Then removes
 * all states that did not get marked. */
void PdaGraph::removeUnreachableStates()
{
	/* Mark all the states that can be reached 
	 * through the existing set of entry points. */
	if ( startState != 0 )
		markReachableFromHere( startState );

	for ( PdaStateSet::Iter si = entryStateSet; si.lte(); si++ )
		markReachableFromHere( *si );

	/* Delete all states that are not marked
	 * and unmark the ones that are marked. */
	PdaState *state = stateList.head;
	while ( state ) {
		PdaState *next = state->next;

		if ( state->stateBits & SB_ISMARKED )
			state->stateBits &= ~ SB_ISMARKED;
		else {
			detachState( state );
			stateList.detach( state );
			delete state;
		}

		state = next;
	}
}
