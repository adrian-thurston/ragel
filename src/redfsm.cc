/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
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

#include <iostream>
#include <sstream>
#include "redfsm.h"
#include "avlmap.h"
#include "mergesort.h"
#include "fsmgraph.h"
#include "parsetree.h"

using std::ostringstream;

string nameOrLoc( GenAction *genAction )
{
	if ( genAction->name != 0 )
		return string(genAction->name);
	else {
		ostringstream ret;
		ret << genAction->loc.line << ":" << genAction->loc.col;
		return ret.str();
	}
}

RedFsm::RedFsm()
:
	wantComplete(false),
	forcedErrorState(false),
	nextActionId(0),
	nextTransId(0),
	errState(0),
	errTrans(0),
	firstFinState(0),
	numFinStates(0),
	allActions(0),
	allActionTables(0),
	allStates(0),
	bAnyToStateActions(false),
	bAnyFromStateActions(false),
	bAnyRegActions(false),
	bAnyEofActions(false),
	bAnyActionGotos(false),
	bAnyActionCalls(false),
	bAnyActionRets(false),
	bAnyRegActionRets(false),
	bAnyRegActionByValControl(false),
	bAnyRegNextStmt(false),
	bAnyRegCurStateRef(false),
	bAnyRegBreak(false),
	bAnyLmSwitchError(false),
	bAnyConditions(false)
{
}

/* Does the machine have any actions. */
bool RedFsm::anyActions()
{
	return actionMap.length() > 0;
}

void RedFsm::depthFirstOrdering( RedState *state )
{
	/* Nothing to do if the state is already on the list. */
	if ( state->onStateList )
		return;

	/* Doing depth first, put state on the list. */
	state->onStateList = true;
	stateList.append( state );
	
//	/* At this point transitions should only be in ranges. */
//	assert( state->outSingle.length() == 0 );
//	assert( state->defTrans == 0 );

	/* Recurse on singles. */
	for ( RedTransList::Iter stel = state->outSingle; stel.lte(); stel++ ) {
		if ( stel->value->targ != 0 )
			depthFirstOrdering( stel->value->targ );
	}

	/* Recurse on everything ranges. */
	for ( RedTransList::Iter rtel = state->outRange; rtel.lte(); rtel++ ) {
		if ( rtel->value->targ != 0 )
			depthFirstOrdering( rtel->value->targ );
	}

	if ( state->defTrans != 0 && state->defTrans->targ != 0 )
		depthFirstOrdering( state->defTrans->targ );
}

/* Ordering states by transition connections. */
void RedFsm::depthFirstOrdering()
{
	/* Init on state list flags. */
	for ( RedStateList::Iter st = stateList; st.lte(); st++ )
		st->onStateList = false;
	
	/* Clear out the state list, we will rebuild it. */
	int stateListLen = stateList.length();
	stateList.abandon();

	/* Add back to the state list from the start state and all other entry
	 * points. */
	depthFirstOrdering( startState );
	for ( RedStateSet::Iter en = entryPoints; en.lte(); en++ )
		depthFirstOrdering( *en );
	if ( forcedErrorState )
		depthFirstOrdering( errState );
	
	/* Make sure we put everything back on. */
	assert( stateListLen == stateList.length() );
}

/* Assign state ids by appearance in the state list. */
void RedFsm::sequentialStateIds()
{
	/* Table based machines depend on the state numbers starting at zero. */
	nextStateId = 0;
	for ( RedStateList::Iter st = stateList; st.lte(); st++ )
		st->id = nextStateId++;
}

/* Stable sort the states by final state status. */
void RedFsm::sortStatesByFinal()
{
	/* Move forward through the list and throw final states onto the end. */
	RedState *state = 0;
	RedState *next = stateList.head;
	RedState *last = stateList.tail;
	while ( state != last ) {
		/* Move forward and load up the next. */
		state = next;
		next = state->next;

		/* Throw to the end? */
		if ( state->isFinal ) {
			stateList.detach( state );
			stateList.append( state );
		}
	}
}

/* Assign state ids by final state state status. */
void RedFsm::sortStateIdsByFinal()
{
	/* Table based machines depend on this starting at zero. */
	nextStateId = 0;

	/* First pass to assign non final ids. */
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		if ( ! st->isFinal ) 
			st->id = nextStateId++;
	}

	/* Second pass to assign final ids. */
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		if ( st->isFinal ) 
			st->id = nextStateId++;
	}
}

struct CmpStateById
{
	static int compare( RedState *st1, RedState *st2 )
	{
		if ( st1->id < st2->id )
			return -1;
		else if ( st1->id > st2->id )
			return 1;
		else
			return 0;
	}
};

void RedFsm::sortByStateId()
{
	/* Make the array. */
	int pos = 0;
	RedState **ptrList = new RedState*[stateList.length()];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ )
		ptrList[pos++] = st;
	
	MergeSort<RedState*, CmpStateById> mergeSort;
	mergeSort.sort( ptrList, stateList.length() );

	stateList.abandon();
	for ( int st = 0; st < pos; st++ )
		stateList.append( ptrList[st] );

	delete[] ptrList;
}

/* Find the final state with the lowest id. */
void RedFsm::findFirstFinState()
{
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		if ( st->isFinal && (firstFinState == 0 || st->id < firstFinState->id) )
			firstFinState = st;
	}
}

void RedFsm::assignActionLocs()
{
	int nextLocation = 0;
	for ( GenActionTableMap::Iter act = actionMap; act.lte(); act++ ) {
		/* Store the loc, skip over the array and a null terminator. */
		act->location = nextLocation;
		nextLocation += act->key.length() + 1;		
	}
}

/* Check if we can extend the current range by displacing any ranges
 * ahead to the singles. */
bool RedFsm::canExtend( const RedTransList &list, int pos )
{
	/* Get the transition that we want to extend. */
	RedTrans *extendTrans = list[pos].value;

	/* Look ahead in the transition list. */
	for ( int next = pos + 1; next < list.length(); pos++, next++ ) {
		/* If they are not continuous then cannot extend. */
		Key nextKey = list[next].lowKey;
		nextKey.decrement();
		if ( list[pos].highKey != nextKey )
			break;

		/* Check for the extenstion property. */
		if ( extendTrans == list[next].value )
			return true;

		/* If the span of the next element is more than one, then don't keep
		 * checking, it won't be moved to single. */
		unsigned long long nextSpan = keyOps->span( list[next].lowKey, list[next].highKey );
		if ( nextSpan > 1 )
			break;
	}
	return false;
}

/* Move ranges to the singles list. */
void RedFsm::moveTransToSingle( RedState *state )
{
	RedTransList &range = state->outRange;
	RedTransList &single = state->outSingle;
	for ( int rpos = 0; rpos < range.length(); ) {
		/* Check if this is a range we can extend. */
		if ( canExtend( range, rpos ) ) {
			/* Transfer singles over. */
			while ( range[rpos].value != range[rpos+1].value ) {
				/* Transfer the range to single. */
				single.append( range[rpos+1] );
				range.remove( rpos+1 );
			}
			
			/* Extend. */
			range[rpos].highKey = range[rpos+1].highKey;
			range.remove( rpos+1 );
		}
		/* Maybe move it to the singles. */
		else if ( keyOps->span( range[rpos].lowKey, range[rpos].highKey ) == 1 ) {
			single.append( range[rpos] );
			range.remove( rpos );
		}
		else {
			/* Keeping it in the ranges. */
			rpos += 1;
		}
	}
}

/* Look through ranges and choose suitable single character transitions. */
void RedFsm::chooseSingle()
{
	/* Loop the states. */
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		/* Rewrite the transition list taking out the suitable single
		 * transtions. */
		moveTransToSingle( st );
	}
}

void RedFsm::makeFlat()
{
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		st->condLowKey = 0;
		st->condHighKey = 0;

		if ( st->outRange.length() == 0 ) {
			st->lowKey = st->highKey = 0;
			st->transList = 0;
		}
		else {
			st->lowKey = st->outRange[0].lowKey;
			st->highKey = st->outRange[st->outRange.length()-1].highKey;
			unsigned long long span = keyOps->span( st->lowKey, st->highKey );
			st->transList = new RedTrans*[ span ];
			memset( st->transList, 0, sizeof(RedTrans*)*span );
			
			for ( RedTransList::Iter trans = st->outRange; trans.lte(); trans++ ) {
				unsigned long long base, trSpan;
				base = keyOps->span( st->lowKey, trans->lowKey )-1;
				trSpan = keyOps->span( trans->lowKey, trans->highKey );
				for ( unsigned long long pos = 0; pos < trSpan; pos++ )
					st->transList[base+pos] = trans->value;
			}

			/* Fill in the gaps with the default transition. */
			for ( unsigned long long pos = 0; pos < span; pos++ ) {
				if ( st->transList[pos] == 0 )
					st->transList[pos] = st->defTrans;
			}
		}
	}
}


/* A default transition has been picked, move it from the outRange to the
 * default pointer. */
void RedFsm::moveToDefault( RedTrans *defTrans, RedState *state )
{
	/* Rewrite the outRange, omitting any ranges that use 
	 * the picked default. */
	RedTransList outRange;
	for ( RedTransList::Iter rtel = state->outRange; rtel.lte(); rtel++ ) {
		/* If it does not take the default, copy it over. */
		if ( rtel->value != defTrans )
			outRange.append( *rtel );
	}

	/* Save off the range we just created into the state's range. */
	state->outRange.transfer( outRange );

	/* Store the default. */
	state->defTrans = defTrans;
}

bool RedFsm::alphabetCovered( RedTransList &outRange )
{
	/* Cannot cover without any out ranges. */
	if ( outRange.length() == 0 )
		return false;

	/* If the first range doesn't start at the the lower bound then the
	 * alphabet is not covered. */
	RedTransList::Iter rtel = outRange;
	if ( keyOps->minKey < rtel->lowKey )
		return false;

	/* Check that every range is next to the previous one. */
	rtel.increment();
	for ( ; rtel.lte(); rtel++ ) {
		Key highKey = rtel[-1].highKey;
		highKey.increment();
		if ( highKey != rtel->lowKey )
			return false;
	}

	/* The last must extend to the upper bound. */
	RedTransEl *last = &outRange[outRange.length()-1];
	if ( last->highKey < keyOps->maxKey )
		return false;

	return true;
}

RedTrans *RedFsm::chooseDefaultSpan( RedState *state )
{
	/* Make a set of transitions from the outRange. */
	RedTransPtrSet stateTransSet;
	for ( RedTransList::Iter rtel = state->outRange; rtel.lte(); rtel++ )
		stateTransSet.insert( rtel->value );
	
	/* For each transition in the find how many alphabet characters the
	 * transition spans. */
	unsigned long long *span = new unsigned long long[stateTransSet.length()];
	memset( span, 0, sizeof(unsigned long long) * stateTransSet.length() );
	for ( RedTransList::Iter rtel = state->outRange; rtel.lte(); rtel++ ) {
		/* Lookup the transition in the set. */
		RedTrans **inSet = stateTransSet.find( rtel->value );
		int pos = inSet - stateTransSet.data;
		span[pos] += keyOps->span( rtel->lowKey, rtel->highKey );
	}

	/* Find the max span, choose it for making the default. */
	RedTrans *maxTrans = 0;
	unsigned long long maxSpan = 0;
	for ( RedTransPtrSet::Iter rtel = stateTransSet; rtel.lte(); rtel++ ) {
		if ( span[rtel.pos()] > maxSpan ) {
			maxSpan = span[rtel.pos()];
			maxTrans = *rtel;
		}
	}

	delete[] span;
	return maxTrans;
}

/* Pick default transitions from ranges for the states. */
void RedFsm::chooseDefaultSpan()
{
	/* Loop the states. */
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		/* Only pick a default transition if the alphabet is covered. This
		 * avoids any transitions in the out range that go to error and avoids
		 * the need for an ERR state. */
		if ( alphabetCovered( st->outRange ) ) {
			/* Pick a default transition by largest span. */
			RedTrans *defTrans = chooseDefaultSpan( st );

			/* Rewrite the transition list taking out the transition we picked
			 * as the default and store the default. */
			moveToDefault( defTrans, st );
		}
	}
}

RedTrans *RedFsm::chooseDefaultGoto( RedState *state )
{
	/* Make a set of transitions from the outRange. */
	RedTransPtrSet stateTransSet;
	for ( RedTransList::Iter rtel = state->outRange; rtel.lte(); rtel++ ) {
		if ( rtel->value->targ == state->next )
			return rtel->value;
	}
	return 0;
}

void RedFsm::chooseDefaultGoto()
{
	/* Loop the states. */
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		/* Pick a default transition. */
		RedTrans *defTrans = chooseDefaultGoto( st );
		if ( defTrans == 0 )
			defTrans = chooseDefaultSpan( st );

		/* Rewrite the transition list taking out the transition we picked
		 * as the default and store the default. */
		moveToDefault( defTrans, st );
	}
}

RedTrans *RedFsm::chooseDefaultNumRanges( RedState *state )
{
	/* Make a set of transitions from the outRange. */
	RedTransPtrSet stateTransSet;
	for ( RedTransList::Iter rtel = state->outRange; rtel.lte(); rtel++ )
		stateTransSet.insert( rtel->value );
	
	/* For each transition in the find how many ranges use the transition. */
	int *numRanges = new int[stateTransSet.length()];
	memset( numRanges, 0, sizeof(int) * stateTransSet.length() );
	for ( RedTransList::Iter rtel = state->outRange; rtel.lte(); rtel++ ) {
		/* Lookup the transition in the set. */
		RedTrans **inSet = stateTransSet.find( rtel->value );
		numRanges[inSet - stateTransSet.data] += 1;
	}

	/* Find the max number of ranges. */
	RedTrans *maxTrans = 0;
	int maxNumRanges = 0;
	for ( RedTransPtrSet::Iter rtel = stateTransSet; rtel.lte(); rtel++ ) {
		if ( numRanges[rtel.pos()] > maxNumRanges ) {
			maxNumRanges = numRanges[rtel.pos()];
			maxTrans = *rtel;
		}
	}

	delete[] numRanges;
	return maxTrans;
}

void RedFsm::chooseDefaultNumRanges()
{
	/* Loop the states. */
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		/* Pick a default transition. */
		RedTrans *defTrans = chooseDefaultNumRanges( st );

		/* Rewrite the transition list taking out the transition we picked
		 * as the default and store the default. */
		moveToDefault( defTrans, st );
	}
}

RedTrans *RedFsm::getErrorTrans( )
{
	/* If the error trans has not been made aready, make it. */
	if ( errTrans == 0 ) {
		/* This insert should always succeed since no transition created by
		 * the user can point to the error state. */
		errTrans = new RedTrans( getErrorState(), 0, nextTransId++ );
		RedTrans *inRes = transSet.insert( errTrans );
		assert( inRes != 0 );
	}
	return errTrans;
}

RedState *RedFsm::getErrorState()
{
	/* Something went wrong. An error state is needed but one was not supplied
	 * by the frontend. */
	assert( errState != 0 );
	return errState;
}


RedTrans *RedFsm::allocateTrans( RedState *targ, RedAction *action )
{
	/* Create a reduced trans and look for it in the transiton set. */
	RedTrans redTrans( targ, action, 0 );
	RedTrans *inDict = transSet.find( &redTrans );
	if ( inDict == 0 ) {
		inDict = new RedTrans( targ, action, nextTransId++ );
		transSet.insert( inDict );
	}
	return inDict;
}

void RedFsm::partitionFsm( int nparts )
{
	/* At this point the states are ordered by a depth-first traversal. We
	 * will allocate to partitions based on this ordering. */
	this->nParts = nparts;
	int partSize = stateList.length() / nparts;
	int remainder = stateList.length() % nparts;
	int numInPart = partSize;
	int partition = 0;
	if ( remainder-- > 0 )
		numInPart += 1;
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		st->partition = partition;

		numInPart -= 1;
		if ( numInPart == 0 ) {
			partition += 1;
			numInPart = partSize;
			if ( remainder-- > 0 )
				numInPart += 1;
		}
	}
}

void RedFsm::setInTrans()
{
	/* First pass counts the number of transitions. */
	for ( RedTransSet::Iter trans = transSet; trans.lte(); trans++ )
		trans->targ->numInTrans += 1;

	/* Pass over states to allocate the needed memory. Reset the counts so we
	 * can use them as the current size. */
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		st->inTrans = new RedTrans*[st->numInTrans];
		st->numInTrans = 0;
	}

	/* Second pass over transitions copies pointers into the in trans list. */
	for ( RedTransSet::Iter trans = transSet; trans.lte(); trans++ )
		trans->targ->inTrans[trans->targ->numInTrans++] = trans;
}

void RedFsm::setValueLimits()
{
	maxSingleLen = 0;
	maxRangeLen = 0;
	maxKeyOffset = 0;
	maxIndexOffset = 0;
	maxActListId = 0;
	maxActionLoc = 0;
	maxActArrItem = 0;
	maxSpan = 0;
	maxCondSpan = 0;
	maxFlatIndexOffset = 0;
	maxCondOffset = 0;
	maxCondLen = 0;
	maxCondSpaceId = 0;
	maxCondIndexOffset = 0;

	/* In both of these cases the 0 index is reserved for no value, so the max
	 * is one more than it would be if they started at 0. */
	maxIndex = transSet.length();
	maxCond = 0;

	/* The nextStateId - 1 is the last state id assigned. */
	maxState = nextStateId - 1;

	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		/* Maximum single length. */
		if ( st->outSingle.length() > maxSingleLen )
			maxSingleLen = st->outSingle.length();

		/* Maximum range length. */
		if ( st->outRange.length() > maxRangeLen )
			maxRangeLen = st->outRange.length();

		/* The key offset index offset for the state after last is not used, skip it.. */
		if ( ! st.last() ) {
			maxKeyOffset += st->outSingle.length() + st->outRange.length()*2;
			maxIndexOffset += st->outSingle.length() + st->outRange.length() + 1;
		}

		/* Max key span. */
		if ( st->transList != 0 ) {
			unsigned long long span = keyOps->span( st->lowKey, st->highKey );
			if ( span > maxSpan )
				maxSpan = span;
		}

		/* Max flat index offset. */
		if ( ! st.last() ) {
			if ( st->transList != 0 )
				maxFlatIndexOffset += keyOps->span( st->lowKey, st->highKey );
			maxFlatIndexOffset += 1;
		}
	}

	for ( GenActionTableMap::Iter at = actionMap; at.lte(); at++ ) {
		/* Maximum id of action lists. */
		if ( at->actListId+1 > maxActListId )
			maxActListId = at->actListId+1;

		/* Maximum location of items in action array. */
		if ( at->location+1 > maxActionLoc )
			maxActionLoc = at->location+1;

		/* Maximum values going into the action array. */
		if ( at->key.length() > maxActArrItem )
			maxActArrItem = at->key.length();
		for ( GenActionTable::Iter item = at->key; item.lte(); item++ ) {
			if ( item->value->actionId > maxActArrItem )
				maxActArrItem = item->value->actionId;
		}
	}
}

void RedFsm::findFinalActionRefs()
{
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		/* Rerence count out of single transitions. */
		for ( RedTransList::Iter rtel = st->outSingle; rtel.lte(); rtel++ ) {
			if ( rtel->value->action != 0 ) {
				rtel->value->action->numTransRefs += 1;
				for ( GenActionTable::Iter item = rtel->value->action->key; item.lte(); item++ )
					item->value->numTransRefs += 1;
			}
		}

		/* Reference count out of range transitions. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			if ( rtel->value->action != 0 ) {
				rtel->value->action->numTransRefs += 1;
				for ( GenActionTable::Iter item = rtel->value->action->key; item.lte(); item++ )
					item->value->numTransRefs += 1;
			}
		}

		/* Reference count default transition. */
		if ( st->defTrans != 0 && st->defTrans->action != 0 ) {
			st->defTrans->action->numTransRefs += 1;
			for ( GenActionTable::Iter item = st->defTrans->action->key; item.lte(); item++ )
				item->value->numTransRefs += 1;
		}

		/* Reference count to state actions. */
		if ( st->toStateAction != 0 ) {
			st->toStateAction->numToStateRefs += 1;
			for ( GenActionTable::Iter item = st->toStateAction->key; item.lte(); item++ )
				item->value->numToStateRefs += 1;
		}

		/* Reference count from state actions. */
		if ( st->fromStateAction != 0 ) {
			st->fromStateAction->numFromStateRefs += 1;
			for ( GenActionTable::Iter item = st->fromStateAction->key; item.lte(); item++ )
				item->value->numFromStateRefs += 1;
		}

		/* Reference count EOF actions. */
		if ( st->eofAction != 0 ) {
			st->eofAction->numEofRefs += 1;
			for ( GenActionTable::Iter item = st->eofAction->key; item.lte(); item++ )
				item->value->numEofRefs += 1;
		}
	}
}

void RedFsm::analyzeAction( GenAction *act, InlineList *inlineList )
{
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		/* Check for various things in regular actions. */
		if ( act->numTransRefs > 0 || act->numToStateRefs > 0 || 
				act->numFromStateRefs > 0 || act->numEofRefs > 0 )
		{
			if ( item->type == InlineItem::LmSwitch && 
					item->tokenRegion->lmSwitchHandlesError )
			{
				bAnyLmSwitchError = true;
			}
		}

		if ( item->children != 0 )
			analyzeAction( act, item->children );
	}
}

void RedFsm::analyzeActionList( RedAction *redAct, InlineList *inlineList )
{
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		if ( item->children != 0 )
			analyzeActionList( redAct, item->children );
	}
}

/* Assign ids to referenced actions. */
void RedFsm::assignActionIds()
{
	int nextActionId = 0;
	for ( GenActionList::Iter act = genActionList; act.lte(); act++ ) {
		/* Only ever interested in referenced actions. */
		if ( numRefs( act ) > 0 )
			act->actionId = nextActionId++;
	}
}

/* Gather various info on the machine. */
void RedFsm::analyzeMachine()
{
	/* Find the true count of action references.  */
	findFinalActionRefs();

	/* Check if there are any calls in action code. */
	for ( GenActionList::Iter act = genActionList; act.lte(); act++ ) {
		/* Record the occurrence of various kinds of actions. */
		if ( act->numToStateRefs > 0 )
			bAnyToStateActions = true;
		if ( act->numFromStateRefs > 0 )
			bAnyFromStateActions = true;
		if ( act->numEofRefs > 0 )
			bAnyEofActions = true;
		if ( act->numTransRefs > 0 )
			bAnyRegActions = true;

		/* Recurse through the action's parse tree looking for various things. */
		analyzeAction( act, act->inlineList );
	}

	/* Analyze reduced action lists. */
	for ( GenActionTableMap::Iter redAct = actionMap; redAct.lte(); redAct++ ) {
		for ( GenActionTable::Iter act = redAct->key; act.lte(); act++ )
			analyzeActionList( redAct, act->value->inlineList );
	}

	/* Find states that have transitions with actions that have next
	 * statements. */
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		/* Check any actions out of outSinge. */
		for ( RedTransList::Iter rtel = st->outSingle; rtel.lte(); rtel++ ) {
			if ( rtel->value->action != 0 && rtel->value->action->anyCurStateRef() )
				st->bAnyRegCurStateRef = true;
		}

		/* Check any actions out of outRange. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			if ( rtel->value->action != 0 && rtel->value->action->anyCurStateRef() )
				st->bAnyRegCurStateRef = true;
		}

		/* Check any action out of default. */
		if ( st->defTrans != 0 && st->defTrans->action != 0 && 
				st->defTrans->action->anyCurStateRef() )
			st->bAnyRegCurStateRef = true;
	}

	/* Assign ids to actions that are referenced. */
	assignActionIds();

	/* Set the maximums of various values used for deciding types. */
	setValueLimits();
}

int transAction( RedTrans *trans )
{
	int retAct = 0;
	if ( trans->action != 0 )
		retAct = trans->action->location+1;
	return retAct;
}

int toStateAction( RedState *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	return act;
}

int fromStateAction( RedState *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	return act;
}

int eofAction( RedState *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	return act;
}


fsm_tables *RedFsm::makeFsmTables()
{
	/* The fsm runtime needs states sorted by id. */
	sortByStateId();

	int pos, curKeyOffset, curIndOffset;
	fsm_tables *fsmTables = new fsm_tables;
	fsmTables->num_states = stateList.length();

	/*
	 * actions
	 */

	fsmTables->num_actions = 1;
	for ( GenActionTableMap::Iter act = actionMap; act.lte(); act++ )
		fsmTables->num_actions += 1 + act->key.length();

	pos = 0;
	fsmTables->actions = new long[fsmTables->num_actions];
	fsmTables->actions[pos++] = 0;
	for ( GenActionTableMap::Iter act = actionMap; act.lte(); act++ ) {
		fsmTables->actions[pos++] = act->key.length();
		for ( GenActionTable::Iter item = act->key; item.lte(); item++ )
			fsmTables->actions[pos++] = item->value->actionId;
	}

	/*
	 * keyOffset
	 */
	pos = 0, curKeyOffset = 0;
	fsmTables->key_offsets = new long[fsmTables->num_states];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		/* Store the current offset. */
		fsmTables->key_offsets[pos++] = curKeyOffset;

		/* Move the key offset ahead. */
		curKeyOffset += st->outSingle.length() + st->outRange.length()*2;
	}

	/*
	 * transKeys
	 */
	fsmTables->num_trans_keys = 0;
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		fsmTables->num_trans_keys += st->outSingle.length();
		fsmTables->num_trans_keys += 2 * st->outRange.length();
	}

	pos = 0;
	fsmTables->trans_keys = new char[fsmTables->num_trans_keys];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ )
			fsmTables->trans_keys[pos++] = stel->lowKey.getVal();
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			fsmTables->trans_keys[pos++] = rtel->lowKey.getVal();
			fsmTables->trans_keys[pos++] = rtel->highKey.getVal();
		}
	}

	/*
	 * singleLengths
	 */
	pos = 0;
	fsmTables->single_lengths = new long[fsmTables->num_states];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ )
		fsmTables->single_lengths[pos++] = st->outSingle.length();

	/*
	 * rangeLengths
	 */
	pos = 0;
	fsmTables->range_lengths = new long[fsmTables->num_states];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ )
		fsmTables->range_lengths[pos++] = st->outRange.length();

	/*
	 * indexOffsets
	 */
	pos = 0, curIndOffset = 0;
	fsmTables->index_offsets = new long[fsmTables->num_states];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		fsmTables->index_offsets[pos++] = curIndOffset;

		curIndOffset += st->outSingle.length() + st->outRange.length();
		if ( st->defTrans != 0 )
			curIndOffset += 1;
	}

	/*
	 * transTargsWI
	 */
	fsmTables->numTransTargsWI = 0;
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		fsmTables->numTransTargsWI += st->outSingle.length();
		fsmTables->numTransTargsWI += st->outRange.length();
		if ( st->defTrans != 0 )
			fsmTables->numTransTargsWI += 1;
	}

	pos = 0;
	fsmTables->transTargsWI = new long[fsmTables->numTransTargsWI];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ )
			fsmTables->transTargsWI[pos++] = stel->value->targ->id;

		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ )
			fsmTables->transTargsWI[pos++] = rtel->value->targ->id;

		if ( st->defTrans != 0 )
			fsmTables->transTargsWI[pos++] = st->defTrans->targ->id;
	}

	/*
	 * transActionsWI
	 */
	fsmTables->numTransActionsWI = 0;
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		fsmTables->numTransActionsWI += st->outSingle.length();
		fsmTables->numTransActionsWI += st->outRange.length();
		if ( st->defTrans != 0 )
			fsmTables->numTransActionsWI += 1;
	}

	pos = 0;
	fsmTables->transActionsWI = new long[fsmTables->numTransActionsWI];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ )
			fsmTables->transActionsWI[pos++] = transAction( stel->value );

		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ )
			fsmTables->transActionsWI[pos++] = transAction( rtel->value );

		if ( st->defTrans != 0 )
			fsmTables->transActionsWI[pos++] = transAction( st->defTrans );
	}

	/*
	 * toStateActions
	 */
	pos = 0;
	fsmTables->to_state_actions = new long[fsmTables->num_states];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ )
		fsmTables->to_state_actions[pos++] = toStateAction( st );

	/*
	 * fromStateActions
	 */
	pos = 0;
	fsmTables->from_state_actions = new long[fsmTables->num_states];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ )
		fsmTables->from_state_actions[pos++] = fromStateAction( st );

	/*
	 * eofActions
	 */
	pos = 0;
	fsmTables->eof_actions = new long[fsmTables->num_states];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ )
		fsmTables->eof_actions[pos++] = eofAction( st );

	/*
	 * eofTargs
	 */
	pos = 0;
	fsmTables->eof_targs = new long[fsmTables->num_states];
	for ( RedStateList::Iter st = stateList; st.lte(); st++ ) {
		int targ = -1;
		if ( st->eofTrans != 0 )
			targ = st->eofTrans->targ->id;
		fsmTables->eof_targs[pos++] = targ;
	}

	/* Start state. */
	fsmTables->start_state = startState->id;

	/* First final state. */
	fsmTables->first_final = ( firstFinState != 0 ) ?
		firstFinState->id : nextStateId;

	/* The error state. */
	fsmTables->error_state = ( errState != 0 ) ?
		errState->id : -1;

	/* The array pointing to actions. */
	pos = 0;
	fsmTables->num_action_switch = genActionList.length();
	fsmTables->action_switch = new GenAction*[fsmTables->num_action_switch];
	for ( GenActionList::Iter act = genActionList; act.lte(); act++ )
		fsmTables->action_switch[pos++] = act;
	
	/*
	 * entryByRegion
	 */

	fsmTables->num_regions = regionToEntry.length()+1;
	fsmTables->entry_by_region = new long[fsmTables->num_regions];
	fsmTables->entry_by_region[0] = fsmTables->error_state;

	pos = 1;
	for ( RegionToEntry::Iter en = regionToEntry; en.lte(); en++ ) {
		/* Find the entry state from the entry id. */
		RedEntryMapEl *entryMapEl = redEntryMap.find( *en );
		
		/* Save it off. */
		fsmTables->entry_by_region[pos++] = entryMapEl != 0 ? entryMapEl->value 
				: fsmTables->error_state;
	}
	
	return fsmTables;
}


