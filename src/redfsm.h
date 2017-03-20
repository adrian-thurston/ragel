/*
 * Copyright 2001-2006 Adrian Thurston <thurston@colm.net>
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

#ifndef _REDFSM_H
#define _REDFSM_H

#include <assert.h>
#include <string.h>
#include <string>
#include "config.h"
#include "common.h"
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

#define TRANS_ERR_TRANS   0
#define STATE_ERR_STATE   0
#define FUNC_NO_FUNC      0

// #define SCORE_ORDERING 1

using std::string;

struct RedStateAp;
struct GenInlineList;
struct GenAction;
struct FsmCtx;
struct GenCondSpace;
typedef BstSet<int> RedCondKeySet;

/*
 * Inline code tree
 */
struct GenInlineItem
{
	enum Type 
	{
		Text, Goto, Call, Ncall, Next, GotoExpr, CallExpr,
		NcallExpr, NextExpr, Ret, Nret,
		PChar, Char, Hold, Curs, Targs, Entry, Exec, Break, Nbreak,
		LmSwitch, LmExec, LmSetActId, LmSetTokEnd, LmGetTokEnd,
		LmInitAct, LmInitTokStart, LmSetTokStart,
		HostStmt, HostExpr, HostText,
		GenStmt, GenExpr, LmCase, LmHold,
		NfaWrapAction, NfaWrapConds
	};

	GenInlineItem( const InputLoc &loc, Type type ) : 
		loc(loc), targId(0), targState(0), 
		lmId(0), children(0), offset(0),
		wrappedAction(0), type(type) { }
	
	~GenInlineItem();
	
	InputLoc loc;
	std::string data;
	int targId;
	RedStateAp *targState;
	int lmId;
	GenInlineList *children;
	int offset;
	GenAction *wrappedAction;
	GenCondSpace *condSpace;
	RedCondKeySet condKeySet;
	Type type;

	GenInlineItem *prev, *next;
};

/* Normally this would be atypedef, but that would entail including DList from
 * ptreetypes, which should be just typedef forwards. */
struct GenInlineList : public DList<GenInlineItem> { };

struct GenInlineExpr
{
	GenInlineExpr( const InputLoc &loc, GenInlineList *inlineList )
		: loc(loc), inlineList( inlineList ) {}
	
	~GenInlineExpr()
	{
		if ( inlineList != 0 ) {
			inlineList->empty();
			delete inlineList;
		}
	}

	InputLoc loc;
	GenInlineList *inlineList;
};

/* Element in list of actions. Contains the string for the code to exectute. */
struct GenAction 
:
	public DListEl<GenAction>
{
	GenAction( )
	:
		inlineList(0), 
		actionId(0),
		numTransRefs(0),
		numToStateRefs(0),
		numFromStateRefs(0),
		numEofRefs(0),
		numNfaPushRefs(0),
		numNfaRestoreRefs(0),
		numNfaPopActionRefs(0),
		numNfaPopTestRefs(0)
	{
	}

	~GenAction()
	{
		if ( inlineList != 0 ) {
			inlineList->empty();
			delete inlineList;
		}
	}

	/* Data collected during parse. */
	InputLoc loc;
	std::string name;
	GenInlineList *inlineList;
	int actionId;

	string nameOrLoc();

	/* Number of references in the final machine. */
	int numRefs() 
		{ return numTransRefs + numToStateRefs + numFromStateRefs + numEofRefs; }
	int numTransRefs;
	int numToStateRefs;
	int numFromStateRefs;
	int numEofRefs;
	int numNfaPushRefs;
	int numNfaRestoreRefs;
	int numNfaPopActionRefs;
	int numNfaPopTestRefs;
};


/* Forwards. */
struct RedStateAp;
struct StateAp;

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
struct CmpGenActionTableEl
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
typedef CmpSTable< GenActionTableEl, CmpGenActionTableEl > CmpGenActionTable;

/* Set of states. */
typedef BstSet<RedStateAp*> RedStateSet;
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
		numNfaPushRefs(0),
		numNfaRestoreRefs(0),
		numNfaPopActionRefs(0),
		numNfaPopTestRefs(0),
		bAnyNextStmt(false), 
		bAnyCurStateRef(false),
		bAnyBreakStmt(false),
		bUsingAct(false)
	{ }
	
	const GenActionTable &getKey() 
		{ return key; }

	GenActionTable key;
	int actListId;
	int location;
	IntSet *eofRefs;

	/* Number of references in the final machine. */
	int numRefs() 
		{ return numTransRefs + numToStateRefs + numFromStateRefs + numEofRefs; }
	int numTransRefs;
	int numToStateRefs;
	int numFromStateRefs;
	int numEofRefs;
	int numNfaPushRefs;
	int numNfaRestoreRefs;
	int numNfaPopActionRefs;
	int numNfaPopTestRefs;

	bool anyNextStmt() { return bAnyNextStmt; }
	bool anyCurStateRef() { return bAnyCurStateRef; }
	bool anyBreakStmt() { return bAnyBreakStmt; }
	bool usingAct() { return bUsingAct; }

	bool bAnyNextStmt;
	bool bAnyCurStateRef;
	bool bAnyBreakStmt;
	bool bUsingAct;
};

typedef AvlTree<RedAction, GenActionTable, CmpGenActionTable> GenActionTableMap;

struct RedCondPair
{
	int id;
	RedStateAp *targ;
	RedAction *action;
};

struct RedCondAp
:
	public AvlTreeEl<RedCondAp>
{
	RedCondAp( RedStateAp *targ, RedAction *action, int id )
	{
		p.id = id;
		p.targ = targ;
		p.action = action;
	}

	RedCondPair p;
};

struct RedCondEl
{
	CondKey key;
	RedCondAp *value;
};

struct CmpRedCondEl
{
	static int compare( const RedCondEl &el1, const RedCondEl &el2 )
	{
		if ( el1.key < el2.key )
			return -1;
		else if ( el1.key > el2.key )
			return 1;
		else if ( el1.value < el2.value )
			return -1;
		else if ( el1.value > el2.value )
			return 1;
		else
			return 0;
	}
};

typedef Vector< GenAction* > GenCondSet;

struct GenCondSpace
{
	GenCondSpace()
	:
		numTransRefs(0),
		numNfaRefs(0)
	{}

	Key baseKey;
	GenCondSet condSet;
	int condSpaceId;

	long fullSize()
		{ return ( 1 << condSet.length() ); }
	
	long numTransRefs;
	long numNfaRefs;

	GenCondSpace *next, *prev;
};

typedef DList<GenCondSpace> CondSpaceList;

struct RedCondVect
{
	int numConds;
	RedCondEl *outConds;
	RedCondAp *errCond;
};

/* Reduced transition. */
struct RedTransAp
:
	public AvlTreeEl<RedTransAp>
{
	RedTransAp( int id, GenCondSpace *condSpace,
			RedCondEl *outConds, int numConds, RedCondAp *errCond )
	:
		id(id),
		condSpace(condSpace)
	{
		v.outConds = outConds;
		v.numConds = numConds;
		v.errCond = errCond;
	}

	RedTransAp( int id, int condId, RedStateAp *targ, RedAction *action )
	:
		id(id),
		condSpace(0)
	{
		p.id = condId;
		p.targ = targ;
		p.action = action;
	}

	long condFullSize() 
	{
		return condSpace == 0 ? 1 : condSpace->fullSize();
	}

	CondKey outCondKey( int off )
	{
		return condSpace == 0 ? CondKey(0) : v.outConds[off].key;
	}

	RedCondPair *outCond( int off )
	{
		return condSpace == 0 ? &p : &v.outConds[off].value->p;
	}

	int numConds()
	{
		return condSpace == 0 ? 1 : v.numConds;
	}

	RedCondPair *errCond()
	{
		return condSpace == 0 ? 0 : ( v.errCond != 0 ? &v.errCond->p : 0 );
	}

	int id;
	GenCondSpace *condSpace;

	/* Either a pair or a vector of conds. */
	union
	{
		RedCondPair p;
		RedCondVect v;
	};
};

/* Compare of transitions for the final reduction of transitions. Comparison
 * is on target and the pointer to the shared action table. It is assumed that
 * when this is used the action tables have been reduced. */
struct CmpRedTransAp
{
	static int compare( const RedTransAp &t1, const RedTransAp &t2 )
	{
		if ( t1.condSpace < t2.condSpace )
			return -1;
		else if ( t1.condSpace > t2.condSpace )
			return 1;
		else {
			if ( t1.condSpace == 0 ) {
				if ( t1.p.targ < t2.p.targ )
					return -1;
				else if ( t1.p.targ > t2.p.targ )
					return 1;
				else if ( t1.p.action < t2.p.action )
					return -1;
				else if ( t1.p.action > t2.p.action )
					return 1;
				else
					return 0;

			}
			else {
				if ( t1.v.numConds < t2.v.numConds )
					return -1;
				else if ( t1.v.numConds > t2.v.numConds )
					return 1;
				else
				{
					RedCondEl *i1 = t1.v.outConds, *i2 = t2.v.outConds;
					long len = t1.v.numConds, cmpResult;
					for ( long pos = 0; pos < len;
							pos += 1, i1 += 1, i2 += 1 )
					{
						cmpResult = CmpRedCondEl::compare(*i1, *i2);
						if ( cmpResult != 0 )
							return cmpResult;
					}
					return 0;
				}
			}
		}
	}
};

struct CmpRedCondAp
{
	static int compare( const RedCondAp &t1, const RedCondAp &t2 )
	{
		if ( t1.p.targ < t2.p.targ )
			return -1;
		else if ( t1.p.targ > t2.p.targ )
			return 1;
		else if ( t1.p.action < t2.p.action )
			return -1;
		else if ( t1.p.action > t2.p.action )
			return 1;
		else
			return 0;
	}
};

typedef AvlBasic<RedTransAp, CmpRedTransAp> TransApSet;
typedef AvlBasic<RedCondAp, CmpRedCondAp> CondApSet;

/* Element in out range. */
struct RedTransEl
{
	/* Constructors. */
	RedTransEl( Key lowKey, Key highKey, RedTransAp *value ) 
	:
		lowKey(lowKey), 
		highKey(highKey), 
		value(value)
#ifdef SCORE_ORDERING
		, score(0)
#endif
	{ }

	Key lowKey, highKey;
	RedTransAp *value;
#ifdef SCORE_ORDERING
	long long score;
#endif
};

typedef Vector<RedTransEl> RedTransList;
typedef Vector<RedStateAp*> RedStateVect;

typedef BstMapEl<RedStateAp*, unsigned long long> RedSpanMapEl;
typedef BstMap<RedStateAp*, unsigned long long> RedSpanMap;

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

struct Condition
{
	Condition( )
		: key(0), baseKey(0) {}

	Key key;
	Key baseKey;
	GenCondSet condSet;

	Condition *next, *prev;
};
typedef DList<Condition> ConditionList;

struct GenStateCond
{
	Key lowKey;
	Key highKey;

	GenCondSpace *condSpace;

	GenStateCond *prev, *next;
};
typedef DList<GenStateCond> GenStateCondList;
typedef Vector<GenStateCond*> StateCondVect;

struct RedNfaTarg
{
	RedNfaTarg( RedStateAp *state, RedAction *push,
			RedAction *popTest, int order )
	:
		id(0),
		state(state),
		push(push),
		popTest(popTest),
		order(order)
	{}

	long id;
	RedStateAp *state;
	RedAction *push;
	RedAction *popTest;
	int order;
};

struct RedNfaTargCmp
{
	static inline long compare( const RedNfaTarg &k1, const RedNfaTarg &k2 )
	{
		if ( k1.order < k2.order )
			return -1;
		else if ( k1.order > k2.order )
			return 1;
		return 0;
	}
};

typedef Vector<RedNfaTarg> RedNfaTargs;

/* Reduced state. */
struct RedStateAp
{
	RedStateAp()
	: 
		defTrans(0), 
		transList(0), 
		isFinal(false), 
		labelNeeded(false), 
		outNeeded(false), 
		onStateList(false), 
		onListRest(false), 
		toStateAction(0), 
		fromStateAction(0), 
		eofAction(0), 
		eofTrans(0), 
		id(0), 
		bAnyRegCurStateRef(false),
		partitionBoundary(false),
		inConds(0),
		numInConds(0),
		inCondTests(0),
		numInCondTests(0),
		nfaTargs(0)
	{ }

	/* Transitions out. */
	RedTransList outSingle;
	RedTransList outRange;
	RedTransAp *defTrans;

	/* For flat keys. */
	Key lowKey, highKey;
	RedTransAp **transList;
	long long low, high;

	/* The list of states that transitions from this state go to. */
	RedStateVect targStates;

	bool isFinal;
	bool labelNeeded;
	bool outNeeded;
	bool onStateList;
	bool onListRest;
	RedAction *toStateAction;
	RedAction *fromStateAction;
	RedAction *eofAction;
	RedTransAp *eofTrans;
	int id;

	/* Pointers for the list of states. */
	RedStateAp *prev, *next;

	bool anyRegCurStateRef() { return bAnyRegCurStateRef; }
	bool bAnyRegCurStateRef;

	int partition;
	bool partitionBoundary;

	RedCondPair **inConds;
	int numInConds;

	RedTransAp **inCondTests;
	int numInCondTests;

	RedNfaTargs *nfaTargs;
};

/* List of states. */
typedef DList<RedStateAp> RedStateList;

/* Set of reduced transitons. Comparison is by pointer. */
typedef BstSet< RedTransAp*, CmpOrd<RedTransAp*> > RedTransSet;

/* Next version of the fsm machine. */
struct RedFsmAp
{
	RedFsmAp( FsmCtx *fsmCtx, int machineId );
	~RedFsmAp();

	KeyOps *keyOps;
	FsmCtx *fsmCtx;
	int machineId;

	bool forcedErrorState;

	int nextActionId;
	int nextTransId;
	int nextCondId;

	/* Next State Id doubles as the total number of state ids. */
	int nextStateId;

	TransApSet transSet;
	CondApSet condSet;
	GenActionTableMap actionMap;
	RedStateList stateList;
	RedStateSet entryPoints;
	RedStateAp *startState;
	RedStateAp *errState;
	RedTransAp *errTrans;
	RedCondAp *errCond;
	RedTransAp *errActionTrans;
	RedStateAp *firstFinState;
	RedStateAp *allStates;
	int numFinStates;
	int nParts;

	bool bAnyToStateActions;
	bool bAnyFromStateActions;
	bool bAnyRegActions;
	bool bAnyEofActions;
	bool bAnyEofTrans;
	bool bAnyActionGotos;
	bool bAnyActionCalls;
	bool bAnyActionNcalls;
	bool bAnyActionRets;
	bool bAnyActionNrets;
	bool bAnyActionByValControl;
	bool bAnyRegActionRets;
	bool bAnyRegActionByValControl;
	bool bAnyRegNextStmt;
	bool bAnyRegCurStateRef;
	bool bAnyRegBreak;
	bool bAnyRegNbreak;
	bool bUsingAct;
	bool bAnyNfaStates;
	bool bAnyNfaPushPops;
	bool bAnyNfaPushes;
	bool bAnyNfaPops;
	bool bAnyTransCondRefs;
	bool bAnyNfaCondRefs;

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
	int maxFlatIndexOffset;
	Key maxKey;
	int maxCondSpaceId;
	int maxCond;

	bool anyActions();
	bool anyToStateActions()        { return bAnyToStateActions; }
	bool anyFromStateActions()      { return bAnyFromStateActions; }
	bool anyRegActions()            { return bAnyRegActions; }
	bool anyEofActions()            { return bAnyEofActions; }
	bool anyEofTrans()              { return bAnyEofTrans; }
	bool anyActionGotos()           { return bAnyActionGotos; }
	bool anyActionCalls()           { return bAnyActionCalls; }
	bool anyActionNcalls()          { return bAnyActionNcalls; }
	bool anyActionRets()            { return bAnyActionRets; }
	bool anyActionNrets()           { return bAnyActionNrets; }
	bool anyActionByValControl()    { return bAnyActionByValControl; }
	bool anyRegActionRets()         { return bAnyRegActionRets; }
	bool anyRegActionByValControl() { return bAnyRegActionByValControl; }
	bool anyRegNextStmt()           { return bAnyRegNextStmt; }
	bool anyRegCurStateRef()        { return bAnyRegCurStateRef; }
	bool anyRegBreak()              { return bAnyRegBreak; }
	bool usingAct()                 { return bUsingAct; }
	bool anyRegNbreak()				{ return bAnyRegNbreak; }
	bool anyNfaStates()             { return bAnyNfaStates; }

	/* Is is it possible to extend a range by bumping ranges that span only
	 * one character to the singles array. */
	bool canExtend( const RedTransList &list, int pos );

	/* Pick single transitions from the ranges. */
	void moveSelectTransToSingle( RedStateAp *state );
	void moveAllTransToSingle( RedStateAp *state );

	void moveSelectTransToSingle();
	void moveAllTransToSingle();

	void makeFlat();

	/* State low/high, in key space and class space. */
	Key lowKey;
	Key highKey;
	long long nextClass;
	long long *classMap;

	/* Support structs for equivalence class computation. */
	struct EquivClass
	{
		EquivClass( Key lowKey, Key highKey, long long value )
			: lowKey(lowKey), highKey(highKey), value(value) {}

		Key lowKey, highKey;
		long long value;
		EquivClass *prev, *next;
	};

	typedef DList<EquivClass> EquivList;
	typedef BstMap<RedTransAp*, int> EquivAlloc;
	typedef BstMapEl<RedTransAp*, int> EquivAllocEl;

	struct PairKey
	{
		PairKey( long long k1, long long k2 )
			: k1(k1), k2(k2) {}

		long long k1;
		long long k2;
	};

	struct PairKeyCmp
	{
		static inline long compare( const PairKey &k1, const PairKey &k2 )
		{
			if ( k1.k1 < k2.k1 )
				return -1;
			else if ( k1.k1 > k2.k1 )
				return 1;
			if ( k1.k2 < k2.k2 )
				return -1;
			else if ( k1.k2 > k2.k2 )
				return 1;
			else
				return 0;
		}
	};

	typedef BstMap< PairKey, long long, PairKeyCmp > PairKeyMap;
	typedef BstMapEl< PairKey, long long > PairKeyMapEl;

	void characterClass( EquivList &equiv );
	void makeFlatClass();

	/* Move a selected transition from ranges to default. */
	void moveToDefault( RedTransAp *defTrans, RedStateAp *state );

	/* Pick a default transition by largest span. */
	RedTransAp *chooseDefaultSpan( RedStateAp *state );
	void chooseDefaultSpan();

	/* Pick a default transition by most number of ranges. */
	RedTransAp *chooseDefaultNumRanges( RedStateAp *state );
	void chooseDefaultNumRanges();

	/* Pick a default transition tailored towards goto driven machine. */
	RedTransAp *chooseDefaultGoto( RedStateAp *state );
	void chooseDefaultGoto();

	/* Ordering states by transition connections. */
	void optimizeStateOrdering( RedStateAp *state );
	void optimizeStateOrdering();

	/* Ordering states by transition connections. */
	void depthFirstOrdering( RedStateAp *state );
	void depthFirstOrdering();

	void breadthFirstAdd( RedStateAp *state );
	void breadthFirstOrdering();

	void randomizedOrdering();

#ifdef SCORE_ORDERING
	long **scores;
	void scoreSecondPass( RedStateAp *state );
	void scoreOrderingBreadth();
	void readScores();
	void scoreOrderingDepth( RedStateAp *state );
	void scoreOrderingDepth();
#endif

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

	RedCondAp *getErrorCond();
	RedTransAp *getErrorTrans();
	RedStateAp *getErrorState();

	/* Is every char in the alphabet covered? */
	bool alphabetCovered( RedTransList &outRange );

	RedTransAp *allocateTrans( RedStateAp *targ, RedAction *action );
	RedTransAp *allocateTrans( GenCondSpace *condSpace,
			RedCondEl *outConds, int numConds, RedCondAp *errCond );

	RedCondAp *allocateCond( RedStateAp *targState, RedAction *actionTable );

	void partitionFsm( int nParts );

	void setInTrans();
};

#endif
