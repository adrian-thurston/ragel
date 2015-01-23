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


#include "global.h"
#include "redbuild.h"
#include "fsmgraph.h"
#include "redfsm.h"
#include "fsmcodegen.h"
#include <string.h>

using namespace std;

RedFsmBuild::RedFsmBuild( Compiler *pd, FsmGraph *fsm )
:
	pd(pd),
	fsm(fsm),
	nextActionTableId(0),
	startState(-1),
	errState(-1)
{
}

void RedFsmBuild::initActionList( unsigned long length )
{ 
	redFsm->allActions = new GenAction[length];
	memset( redFsm->allActions, 0, sizeof(GenAction) * length );
	for ( unsigned long a = 0; a < length; a++ )
		redFsm->genActionList.append( redFsm->allActions+a );
}


void RedFsmBuild::makeActionList()
{
	/* Determine which actions to write. */
	int nextActionId = 0;
	for ( ActionList::Iter act = pd->actionList; act.lte(); act++ ) {
		if ( act->numRefs() > 0 || act->numCondRefs > 0 )
			act->actionId = nextActionId++;
	}

	initActionList( nextActionId );
	curAction = 0;

	for ( ActionList::Iter act = pd->actionList; act.lte(); act++ ) {
		if ( act->actionId >= 0 )
			makeAction( act );
	}
}

void RedFsmBuild::initActionTableList( unsigned long length )
{ 
	redFsm->allActionTables = new RedAction[length];
}

void RedFsmBuild::initStateList( unsigned long length )
{
	redFsm->allStates = new RedState[length];
	for ( unsigned long s = 0; s < length; s++ )
		redFsm->stateList.append( redFsm->allStates+s );

	/* We get the start state as an offset, set the pointer now. */
	assert( startState >= 0 );
	redFsm->startState = redFsm->allStates + startState;
	if ( errState >= 0 )
		redFsm->errState = redFsm->allStates + errState;
	for ( EntryIdVect::Iter en = redFsm->entryPointIds; en.lte(); en++ )
		redFsm->entryPoints.insert( redFsm->allStates + *en );

	/* The nextStateId is no longer used to assign state ids (they come in set
	 * from the frontend now), however generation code still depends on it.
	 * Should eventually remove this variable. */
	redFsm->nextStateId = redFsm->stateList.length();
}

void RedFsmBuild::addEntryPoint( int entryId, unsigned long entryState )
{
	redFsm->entryPointIds.append( entryState );
	redFsm->redEntryMap.insert( entryId, entryState );
}

void RedFsmBuild::addRegionToEntry( int regionId, int entryId )
{
	assert( regionId == redFsm->regionToEntry.length() );
	redFsm->regionToEntry.append( entryId );
}

void RedFsmBuild::initTransList( int snum, unsigned long length )
{
	/* Could preallocate the out range to save time growing it. For now do
	 * nothing. */
}

void RedFsmBuild::newTrans( int snum, int tnum, Key lowKey, 
		Key highKey, long targ, long action )
{
	/* Get the current state and range. */
	RedState *curState = redFsm->allStates + snum;
	RedTransList &destRange = curState->outRange;

	if ( curState == redFsm->errState )
		return;

	/* Make the new transitions. */
	RedState *targState = targ >= 0 ? (redFsm->allStates + targ) : 
			redFsm->wantComplete ? redFsm->getErrorState() : 0;
	RedAction *actionTable = action >= 0 ? (redFsm->allActionTables + action) : 0;
	RedTrans *trans = redFsm->allocateTrans( targState, actionTable );
	RedTransEl transEl( lowKey, highKey, trans );

	if ( redFsm->wantComplete ) {
		/* If the machine is to be complete then we need to fill any gaps with
		 * the error transitions. */
		if ( destRange.length() == 0 ) {
			/* Range is currently empty. */
			if ( keyOps->minKey < lowKey ) {
				/* The first range doesn't start at the low end. */
				Key fillHighKey = lowKey;
				fillHighKey.decrement();

				/* Create the filler with the state's error transition. */
				RedTransEl newTel( keyOps->minKey, fillHighKey, redFsm->getErrorTrans() );
				destRange.append( newTel );
			}
		}
		else {
			/* The range list is not empty, get the the last range. */
			RedTransEl *last = &destRange[destRange.length()-1];
			Key nextKey = last->highKey;
			nextKey.increment();
			if ( nextKey < lowKey ) {
				/* There is a gap to fill. Make the high key. */
				Key fillHighKey = lowKey;
				fillHighKey.decrement();

				/* Create the filler with the state's error transtion. */
				RedTransEl newTel( nextKey, fillHighKey, redFsm->getErrorTrans() );
				destRange.append( newTel );
			}
		}
	}

	/* Filler taken care of. Append the range. */
	destRange.append( RedTransEl( lowKey, highKey, trans ) );
}

void RedFsmBuild::finishTransList( int snum )
{
	/* Get the current state and range. */
	RedState *curState = redFsm->allStates + snum;
	RedTransList &destRange = curState->outRange;

	if ( curState == redFsm->errState )
		return;

	/* If building a complete machine we may need filler on the end. */
	if ( redFsm->wantComplete ) {
		/* Check if there are any ranges already. */
		if ( destRange.length() == 0 ) {
			/* Fill with the whole alphabet. */
			/* Add the range on the lower and upper bound. */
			RedTransEl newTel( keyOps->minKey, keyOps->maxKey, redFsm->getErrorTrans() );
			destRange.append( newTel );
		}
		else {
			/* Get the last and check for a gap on the end. */
			RedTransEl *last = &destRange[destRange.length()-1];
			if ( last->highKey < keyOps->maxKey ) {
				/* Make the high key. */
				Key fillLowKey = last->highKey;
				fillLowKey.increment();

				/* Create the new range with the error trans and append it. */
				RedTransEl newTel( fillLowKey, keyOps->maxKey, redFsm->getErrorTrans() );
				destRange.append( newTel );
			}
		}
	}
}

void RedFsmBuild::setId( int snum, int id )
{
	RedState *curState = redFsm->allStates + snum;
	curState->id = id;
}

void RedFsmBuild::setEofTrans( int snum, int eofTarget, int actId )
{
	RedState *curState = redFsm->allStates + snum;
	RedState *targState = redFsm->allStates + eofTarget;
	RedAction *eofAct = redFsm->allActionTables + actId;
	curState->eofTrans = redFsm->allocateTrans( targState, eofAct );
}

void RedFsmBuild::setFinal( int snum )
{
	RedState *curState = redFsm->allStates + snum;
	curState->isFinal = true;
}


void RedFsmBuild::setStateActions( int snum, long toStateAction, 
			long fromStateAction, long eofAction )
{
	RedState *curState = redFsm->allStates + snum;
	if ( toStateAction >= 0 )
		curState->toStateAction = redFsm->allActionTables + toStateAction;
	if ( fromStateAction >= 0 )
		curState->fromStateAction = redFsm->allActionTables + fromStateAction;
	if ( eofAction >= 0 )
		curState->eofAction = redFsm->allActionTables + eofAction;
}

void RedFsmBuild::closeMachine()
{
}


void RedFsmBuild::initStateCondList( int snum, ulong length )
{
	/* Could preallocate these, as we could with transitions. */
}

void RedFsmBuild::setForcedErrorState()
{
	redFsm->forcedErrorState = true;
}

Key RedFsmBuild::findMaxKey()
{
	Key maxKey = keyOps->maxKey;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		assert( st->outSingle.length() == 0 );
		assert( st->defTrans == 0 );

		long rangeLen = st->outRange.length();
		if ( rangeLen > 0 ) {
			Key highKey = st->outRange[rangeLen-1].highKey;
			if ( highKey > maxKey )
				maxKey = highKey;
		}
	}
	return maxKey;
}


void RedFsmBuild::makeActionTableList()
{
	/* Must first order the action tables based on their id. */
	int numTables = nextActionTableId;
	RedActionTable **tables = new RedActionTable*[numTables];
	for ( ActionTableMap::Iter at = actionTableMap; at.lte(); at++ )
		tables[at->id] = at;

	initActionTableList( numTables );
	curActionTable = 0;

	for ( int t = 0; t < numTables; t++ ) {
		long length = tables[t]->key.length();

		/* Collect the action table. */
		RedAction *redAct = redFsm->allActionTables + curActionTable;
		redAct->actListId = curActionTable;
		redAct->key.setAsNew( length );

		int pos = 0;
		for ( ActionTable::Iter atel = tables[t]->key; atel.lte(); atel++ ) {
			int actionId = atel->value->actionId;
			redAct->key[pos].key = 0;
			redAct->key[pos].value = redFsm->allActions+actionId;
			pos += 1;
		}

		/* Insert into the action table map. */
		redFsm->actionMap.insert( redAct );

		curActionTable += 1;

	}

	delete[] tables;
}

void RedFsmBuild::reduceActionTables()
{
	/* Reduce the actions tables to a set. */
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		RedActionTable *actionTable = 0;

		/* Reduce To State Actions. */
		if ( st->toStateActionTable.length() > 0 ) {
			if ( actionTableMap.insert( st->toStateActionTable, &actionTable ) )
				actionTable->id = nextActionTableId++;
		}

		/* Reduce From State Actions. */
		if ( st->fromStateActionTable.length() > 0 ) {
			if ( actionTableMap.insert( st->fromStateActionTable, &actionTable ) )
				actionTable->id = nextActionTableId++;
		}

		/* Reduce EOF actions. */
		if ( st->eofActionTable.length() > 0 ) {
			if ( actionTableMap.insert( st->eofActionTable, &actionTable ) )
				actionTable->id = nextActionTableId++;
		}

		/* Loop the transitions and reduce their actions. */
		for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
			if ( trans->actionTable.length() > 0 ) {
				if ( actionTableMap.insert( trans->actionTable, &actionTable ) )
					actionTable->id = nextActionTableId++;
			}
		}
	}
}

void RedFsmBuild::appendTrans( TransListVect &outList, Key lowKey, 
		Key highKey, FsmTrans *trans )
{
	if ( trans->toState != 0 || trans->actionTable.length() > 0 )
		outList.append( TransEl( lowKey, highKey, trans ) );
}

void RedFsmBuild::makeTrans( Key lowKey, Key highKey, FsmTrans *trans )
{
	/* First reduce the action. */
	RedActionTable *actionTable = 0;
	if ( trans->actionTable.length() > 0 )
		actionTable = actionTableMap.find( trans->actionTable );

	long targ = trans->toState == 0 ? -1 : trans->toState->alg.stateNum;
	long action = actionTable == 0 ? -1 : actionTable->id;

	newTrans( curState, curTrans++, lowKey, highKey, targ, action );
}

void RedFsmBuild::makeTransList( FsmState *state )
{
	TransListVect outList;

	/* If there is only are no ranges the task is simple. */
	if ( state->outList.length() > 0 ) {
		/* Loop each source range. */
		for ( TransList::Iter trans = state->outList; trans.lte(); trans++ ) {
			/* Reduce the transition. If it reduced to anything then add it. */
			appendTrans( outList, trans->lowKey, trans->highKey, trans );
		}
	}

	long length = outList.length();
	initTransList( curState, length );
	curTrans = 0;

	for ( TransListVect::Iter tvi = outList; tvi.lte(); tvi++ )
		makeTrans( tvi->lowKey, tvi->highKey, tvi->value );
	finishTransList( curState );
}

void RedFsmBuild::newAction( int anum, char *name, int line, int col, Action *action )
{
	redFsm->allActions[anum].actionId = anum;
	redFsm->allActions[anum].name = name;
	redFsm->allActions[anum].loc.line = line;
	redFsm->allActions[anum].loc.col = col;
	redFsm->allActions[anum].inlineList = action->inlineList;
	redFsm->allActions[anum].objField = action->objField;
	redFsm->allActions[anum].markType = action->markType;
	redFsm->allActions[anum].markId = action->markId + 1;
}

void RedFsmBuild::makeAction( Action *action )
{
	int line = action->loc.line;
	int col = action->loc.col;

	char *name = 0;
	if ( action->name != 0 ) 
		name = action->name;

	newAction( curAction++, name, line, col, action );
}

void xmlEscapeHost( std::ostream &out, char *data, int len )
{
	char *end = data + len;
	while ( data != end ) {
		switch ( *data ) {
		case '<': out << "&lt;"; break;
		case '>': out << "&gt;"; break;
		case '&': out << "&amp;"; break;
		default: out << *data; break;
		}
		data += 1;
	}
}

void RedFsmBuild::makeStateActions( FsmState *state )
{
	RedActionTable *toStateActions = 0;
	if ( state->toStateActionTable.length() > 0 )
		toStateActions = actionTableMap.find( state->toStateActionTable );

	RedActionTable *fromStateActions = 0;
	if ( state->fromStateActionTable.length() > 0 )
		fromStateActions = actionTableMap.find( state->fromStateActionTable );

	RedActionTable *eofActions = 0;
	if ( state->eofActionTable.length() > 0 )
		eofActions = actionTableMap.find( state->eofActionTable );
	
	if ( toStateActions != 0 || fromStateActions != 0 || eofActions != 0 ) {
		long toStateAction = -1;
		long fromStateAction = -1;
		long eofAction = -1;

		if ( toStateActions != 0 )
			toStateAction = toStateActions->id;
		if ( fromStateActions != 0 )
			fromStateAction = fromStateActions->id;
		if ( eofActions != 0 )
			eofAction = eofActions->id;

		setStateActions( curState, toStateAction,
				fromStateAction, eofAction );
	}
}

void RedFsmBuild::makeStateList()
{
	/* Write the list of states. */
	long length = fsm->stateList.length();
	initStateList( length );
	curState = 0;

	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		/* Both or neither should be set. */
		assert( !( (st->eofTarget != 0) xor (st->eofActionTable.length() > 0) ) );

		makeStateActions( st );
		makeTransList( st );

		setId( curState, st->alg.stateNum );
		if ( st->isFinState() )
			setFinal( curState );

		/* If there is an eof target, make an eof transition. */
		if ( st->eofTarget != 0 ) {
			/* Find the eof actions. */
			RedActionTable *eofActions = 0;
			eofActions = actionTableMap.find( st->eofActionTable );
			setEofTrans( curState, st->eofTarget->alg.stateNum, eofActions->id );
		}

		curState += 1;
	}
}

void RedFsmBuild::makeEntryPoints()
{
	if ( fsm->lmRequiresErrorState )
		setForcedErrorState();

	for ( EntryMap::Iter en = fsm->entryPoints; en.lte(); en++ ) {
		/* Get the name instantiation from nameIndex. */
		FsmState *state = en->value;
		long entry = state->alg.stateNum;
		addEntryPoint( en->key, entry );
	}

	for ( RegionList::Iter reg = pd->regionList; reg.lte(); reg++ ) {
		assert( reg->impl->regionNameInst != 0 );

		TokenRegion *use = reg;

		if ( use->zeroLel != 0 )
			use = use->ignoreOnly;

		NameInst *regionName = use->impl->regionNameInst;
		addRegionToEntry( reg->id, regionName->id );
	}
}

void RedFsmBuild::makeMachine()
{
	/* Action tables. */
	reduceActionTables();

	makeActionList();
	makeActionTableList();
	makeConditions();

	/* Start state. */
	startState = fsm->startState->alg.stateNum;
	
	/* Error state. */
	if ( fsm->errState != 0 )
		errState = fsm->errState->alg.stateNum;

	makeEntryPoints();
	makeStateList();
}

void RedFsmBuild::makeConditions()
{
}

RedFsm *RedFsmBuild::reduceMachine()
{
	redFsm = new RedFsm();
	redFsm->wantComplete = true;

	/* Open the definition. */
	makeMachine();

	/* Do this before distributing transitions out to singles and defaults
	 * makes life easier. */
	redFsm->maxKey = findMaxKey();

	redFsm->assignActionLocs();

	/* Find the first final state (The final state with the lowest id). */
	redFsm->findFirstFinState();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* Maybe do flat expand, otherwise choose single. */
	redFsm->chooseSingle();

	/* Set up incoming transitions. */
	redFsm->setInTrans();

	/* Anlayze Machine will find the final action reference counts, among
	 * other things. We will use these in reporting the usage
	 * of fsm directives in action code. */
	redFsm->analyzeMachine();

	return redFsm;
}

