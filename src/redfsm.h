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

#ifndef _REDFSM_H
#define _REDFSM_H

#include <assert.h>
#include <string.h>
#include <string>
#include "keyops.h"
#include "vector.h"
#include "dlist.h"
#include "compare.h"
#include "bstmap.h"
#include "bstset.h"
#include "avlmap.h"
#include "avltree.h"
#include "avlbasic.h"
#include "mergesort.h"
#include "sbstmap.h"
#include "sbstset.h"
#include "sbsttable.h"
#include "global.h"
#include "pdarun.h"

#define TRANS_ERR_TRANS   0
#define STATE_ERR_STATE   0
#define FUNC_NO_FUNC      0

using std::string;

struct RedState;
struct InlineList;
struct Compiler;
struct ObjectField;

/* Element in list of actions. Contains the string for the code to exectute. */
struct GenAction 
{
	/* Data collected during parse. */
	InputLoc loc;
	char *name;
	InlineList *inlineList;
	int actionId;
	MarkType markType;
	ObjectField *objField;
	long markId;

	int numTransRefs;
	int numToStateRefs;
	int numFromStateRefs;
	int numEofRefs;

	GenAction *prev, *next;
};

typedef DList<GenAction> GenActionList;
string nameOrLoc( GenAction *genAction );

/* Number of references in the final machine. */
inline int numRefs( GenAction *genAction ) 
{
	return genAction->numTransRefs + 
		genAction->numToStateRefs + 
		genAction->numFromStateRefs + 
		genAction->numEofRefs;
}


/* Forwards. */
struct RedState;
struct FsmState;

/* Transistion GenAction Element. */
typedef SBstMapEl< int, GenAction* > GenActionTableEl;

/* Transition GenAction Table.  */
struct GenActionTable 
	: public SBstMap< int, GenAction*, CmpOrd<int> >
{
	void setAction( int ordering, GenAction *action );
	void setActions( int *orderings, GenAction **actions, int nActs );
	void setActions( const GenActionTable &other );
};

/* Compare of a whole action table element (key & value). */
struct GenCmpActionTableEl
{
	static int compare( const GenActionTableEl &action1, 
			const GenActionTableEl &action2 )
	{
		if ( action1.key < action2.key )
			return -1;
		else if ( action1.key > action2.key )
			return 1;
		else if ( action1.value < action2.value )
			return -1;
		else if ( action1.value > action2.value )
			return 1;
		return 0;
	}
};

/* Compare for GenActionTable. */
typedef CmpSTable< GenActionTableEl, GenCmpActionTableEl > GenCmpActionTable;

/* Set of states. */
typedef BstSet<RedState*> RedStateSet;
typedef BstSet<int> IntSet;

/* Reduced action. */
struct RedAction
:
	public AvlTreeEl<RedAction>
{
	RedAction( )
	:	
		key(), 
		eofRefs(0),
		numTransRefs(0),
		numToStateRefs(0),
		numFromStateRefs(0),
		numEofRefs(0),
		bAnyNextStmt(false), 
		bAnyCurStateRef(false),
		bAnyBreakStmt(false)
	{ }
	
	const GenActionTable &getKey() 
		{ return key; }

	GenActionTable key;
	int actListId;
	int location;
	IntSet *eofRefs;

	/* Number of references in the final machine. */
	bool numRefs() 
		{ return numTransRefs + numToStateRefs + numFromStateRefs + numEofRefs; }
	int numTransRefs;
	int numToStateRefs;
	int numFromStateRefs;
	int numEofRefs;

	bool anyNextStmt() { return bAnyNextStmt; }
	bool anyCurStateRef() { return bAnyCurStateRef; }
	bool anyBreakStmt() { return bAnyBreakStmt; }

	bool bAnyNextStmt;
	bool bAnyCurStateRef;
	bool bAnyBreakStmt;
};
typedef AvlTree<RedAction, GenActionTable, GenCmpActionTable> GenActionTableMap;

/* Reduced transition. */
struct RedTrans
:
	public AvlTreeEl<RedTrans>
{
	RedTrans( RedState *targ, RedAction *action, int id )
		: targ(targ), action(action), id(id), labelNeeded(true) { }

	RedState *targ;
	RedAction *action;
	int id;
	bool partitionBoundary;
	bool labelNeeded;
};

/* Compare of transitions for the final reduction of transitions. Comparison
 * is on target and the pointer to the shared action table. It is assumed that
 * when this is used the action tables have been reduced. */
struct CmpRedTrans
{
	static int compare( const RedTrans &t1, const RedTrans &t2 )
	{
		if ( t1.targ < t2.targ )
			return -1;
		else if ( t1.targ > t2.targ )
			return 1;
		else if ( t1.action < t2.action )
			return -1;
		else if ( t1.action > t2.action )
			return 1;
		else
			return 0;
	}
};

typedef AvlBasic<RedTrans, CmpRedTrans> RedTransSet;

/* Element in out range. */
struct RedTransEl
{
	/* Constructors. */
	RedTransEl( Key lowKey, Key highKey, RedTrans *value ) 
		: lowKey(lowKey), highKey(highKey), value(value) { }

	Key lowKey, highKey;
	RedTrans *value;
};

typedef Vector<RedTransEl> RedTransList;
typedef Vector<RedState*> RedStateVect;

typedef BstMapEl<RedState*, unsigned long long> RedSpanMapEl;
typedef BstMap<RedState*, unsigned long long> RedSpanMap;

/* Compare used by span map sort. Reverse sorts by the span. */
struct CmpRedSpanMapEl
{
	static int compare( const RedSpanMapEl &smel1, const RedSpanMapEl &smel2 )
	{
		if ( smel1.value > smel2.value )
			return -1;
		else if ( smel1.value < smel2.value )
			return 1;
		else
			return 0;
	}
};

/* Sorting state-span map entries by span. */
typedef MergeSort<RedSpanMapEl, CmpRedSpanMapEl> RedSpanMapSort;

/* Set of entry ids that go into this state. */
typedef Vector<int> EntryIdVect;
typedef Vector<char*> EntryNameVect;

/* Maps entry ids (defined by the frontend, to reduced state ids. */
typedef BstMap<int, int> RedEntryMap;
typedef BstMapEl<int, int> RedEntryMapEl;

typedef Vector<int> RegionToEntry;

/* Reduced state. */
struct RedState
{
	RedState()
	: 
		defTrans(0), 
		transList(0), 
		isFinal(false), 
		labelNeeded(false), 
		outNeeded(false), 
		onStateList(false), 
		toStateAction(0), 
		fromStateAction(0), 
		eofAction(0), 
		eofTrans(0),
		id(0), 
		bAnyRegCurStateRef(false),
		partitionBoundary(false),
		inTrans(0),
		numInTrans(0)
	{ }

	/* Transitions out. */
	RedTransList outSingle;
	RedTransList outRange;
	RedTrans *defTrans;

	/* For flat conditions. */
	Key condLowKey, condHighKey;

	/* For flat keys. */
	Key lowKey, highKey;
	RedTrans **transList;

	/* The list of states that transitions from this state go to. */
	RedStateVect targStates;

	bool isFinal;
	bool labelNeeded;
	bool outNeeded;
	bool onStateList;
	RedAction *toStateAction;
	RedAction *fromStateAction;
	RedAction *eofAction;
	RedTrans *eofTrans;
	int id;

	/* Pointers for the list of states. */
	RedState *prev, *next;

	bool anyRegCurStateRef() { return bAnyRegCurStateRef; }
	bool bAnyRegCurStateRef;

	int partition;
	bool partitionBoundary;

	RedTrans **inTrans;
	int numInTrans;
};

/* List of states. */
typedef DList<RedState> RedStateList;

/* Set of reduced transitons. Comparison is by pointer. */
typedef BstSet< RedTrans*, CmpOrd<RedTrans*> > RedTransPtrSet;

/* Next version of the fsm machine. */
struct RedFsm
{
	RedFsm();

	bool wantComplete;
	bool forcedErrorState;

	int nextActionId;
	int nextTransId;

	/* Next State Id doubles as the total number of state ids. */
	int nextStateId;

	RedTransSet transSet;
	GenActionTableMap actionMap;
	RedStateList stateList;
	RedStateSet entryPoints;
	RedState *startState;
	RedState *errState;
	RedTrans *errTrans;
	RedTrans *errActionTrans;
	RedState *firstFinState;
	int numFinStates;
	int nParts;

	GenAction *allActions;
	RedAction *allActionTables;
	RedState *allStates;
	GenActionList genActionList;
	EntryIdVect entryPointIds;
	RedEntryMap redEntryMap;
	RegionToEntry regionToEntry;

	bool bAnyToStateActions;
	bool bAnyFromStateActions;
	bool bAnyRegActions;
	bool bAnyEofActions;
	bool bAnyActionGotos;
	bool bAnyActionCalls;
	bool bAnyActionRets;
	bool bAnyRegActionRets;
	bool bAnyRegActionByValControl;
	bool bAnyRegNextStmt;
	bool bAnyRegCurStateRef;
	bool bAnyRegBreak;
	bool bAnyLmSwitchError;
	bool bAnyConditions;

	int maxState;
	int maxSingleLen;
	int maxRangeLen;
	int maxKeyOffset;
	int maxIndexOffset;
	int maxIndex;
	int maxActListId;
	int maxActionLoc;
	int maxActArrItem;
	unsigned long long maxSpan;
	unsigned long long maxCondSpan;
	int maxFlatIndexOffset;
	Key maxKey;
	int maxCondOffset;
	int maxCondLen;
	int maxCondSpaceId;
	int maxCondIndexOffset;
	int maxCond;

	bool anyActions();
	bool anyToStateActions()        { return bAnyToStateActions; }
	bool anyFromStateActions()      { return bAnyFromStateActions; }
	bool anyRegActions()            { return bAnyRegActions; }
	bool anyEofActions()            { return bAnyEofActions; }
	bool anyActionGotos()           { return bAnyActionGotos; }
	bool anyActionCalls()           { return bAnyActionCalls; }
	bool anyActionRets()            { return bAnyActionRets; }
	bool anyRegActionRets()         { return bAnyRegActionRets; }
	bool anyRegActionByValControl() { return bAnyRegActionByValControl; }
	bool anyRegNextStmt()           { return bAnyRegNextStmt; }
	bool anyRegCurStateRef()        { return bAnyRegCurStateRef; }
	bool anyRegBreak()              { return bAnyRegBreak; }
	bool anyLmSwitchError()         { return bAnyLmSwitchError; }
	bool anyConditions()            { return bAnyConditions; }

	/* Is is it possible to extend a range by bumping ranges that span only
	 * one character to the singles array. */
	bool canExtend( const RedTransList &list, int pos );

	/* Pick single transitions from the ranges. */
	void moveTransToSingle( RedState *state );
	void chooseSingle();

	void makeFlat();

	/* Move a selected transition from ranges to default. */
	void moveToDefault( RedTrans *defTrans, RedState *state );

	/* Pick a default transition by largest span. */
	RedTrans *chooseDefaultSpan( RedState *state );
	void chooseDefaultSpan();

	/* Pick a default transition by most number of ranges. */
	RedTrans *chooseDefaultNumRanges( RedState *state );
	void chooseDefaultNumRanges();

	/* Pick a default transition tailored towards goto driven machine. */
	RedTrans *chooseDefaultGoto( RedState *state );
	void chooseDefaultGoto();

	/* Ordering states by transition connections. */
	void optimizeStateOrdering( RedState *state );
	void optimizeStateOrdering();

	/* Ordering states by transition connections. */
	void depthFirstOrdering( RedState *state );
	void depthFirstOrdering();

	/* Set state ids. */
	void sequentialStateIds();
	void sortStateIdsByFinal();

	/* Arrange states in by final id. This is a stable sort. */
	void sortStatesByFinal();

	/* Sorting states by id. */
	void sortByStateId();

	/* Locating the first final state. This is the final state with the lowest
	 * id. */
	void findFirstFinState();

	void assignActionLocs();

	RedTrans *getErrorTrans();
	RedState *getErrorState();

	/* Is every char in the alphabet covered? */
	bool alphabetCovered( RedTransList &outRange );

	RedTrans *allocateTrans( RedState *targState, RedAction *actionTable );

	void partitionFsm( int nParts );

	void setInTrans();
	void setValueLimits();
	void assignActionIds();
	void analyzeActionList( RedAction *redAct, InlineList *inlineList );
	void analyzeAction( GenAction *act, InlineList *inlineList );
	void findFinalActionRefs();
	void analyzeMachine();

	fsm_tables *makeFsmTables();
};


#endif /* _REDFSM_H */
