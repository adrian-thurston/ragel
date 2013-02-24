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

#ifndef _PDAGRAPH_H
#define _PDAGRAPH_H

#include <assert.h>
#include "vector.h"
#include "bstset.h"
#include "compare.h"
#include "avltree.h"
#include "dlist.h"
#include "bstmap.h"
#include "sbstmap.h"
#include "sbstset.h"
#include "sbsttable.h"
#include "avlset.h"
#include "dlistmel.h"
#include "avltree.h"

/* Flags for states. */
#define SB_ISFINAL    0x04
#define SB_ISMARKED   0x08
#define SB_ISSTART    0x10

/* Flags for transitions. */
#define TB_ISMARKED   0x01

struct PdaTrans;
struct PdaState;
struct PdaGraph;
struct TokenInstance;
struct Production;
struct LangEl;
struct TokenRegion;

typedef Vector<TokenRegion*> RegionVect;

typedef Vector<long> ActDataList;

struct ActionData
{
	ActionData( int targ, ActDataList &actions, int commitLen )
		: targ(targ), commitLen(commitLen), id(0), actions(actions) { }

	int targ;
	int commitLen;
	int id;

	ActDataList actions;
};


struct CmpActionData
{
	static int compare( const ActionData &ap1, const ActionData &ap2 )
	{
		if ( ap1.targ < ap2.targ )
			return -1;
		else if ( ap1.targ > ap2.targ )
			return 1;
		else if ( ap1.commitLen < ap2.commitLen )
			return -1;
		else if ( ap1.commitLen > ap2.commitLen )
			return 1;
		else if ( ap1.id < ap2.id )
			return -1;
		else if ( ap1.id > ap2.id )
			return 1;

		return CmpTable< long, CmpOrd<long> >::
			compare( ap1.actions, ap2.actions );
	}
};

typedef AvlSet<ActionData, CmpActionData> PdaActionSet;
typedef AvlSetEl<ActionData> PdaActionSetEl;

/* List pointers for the closure queue. Goes into state. */
struct ClosureQueueListEl { PdaState *prev, *next; };

/* Queue of states, transitions to be closed. */
typedef DListMel< PdaState, ClosureQueueListEl > StateClosureQueue;
typedef DList<PdaTrans> TransClosureQueue;

typedef BstSet< Production*, CmpOrd<Production*> > DefSet;
typedef CmpTable< Production*, CmpOrd<Production*> > CmpDefSet;
typedef BstSet< DefSet, CmpDefSet > DefSetSet;

typedef Vector< Production* > DefVect;
typedef BstSet< long, CmpOrd<long> > AlphSet;

struct ExpandToEl
{
	ExpandToEl( PdaState *state, int prodId )
		: state(state), prodId(prodId) { }

	PdaState *state;
	int prodId;
};

struct CmpExpandToEl
{
	static inline int compare( const ExpandToEl &etel1, const ExpandToEl &etel2 )
	{ 
		if ( etel1.state < etel2.state )
			return -1;
		else if ( etel1.state > etel2.state )
			return 1;
		else if ( etel1.prodId < etel2.prodId )
			return -1;
		else if ( etel1.prodId > etel2.prodId )
			return 1;
		else
			return 0;
	}
};

typedef BstSet<ExpandToEl, CmpExpandToEl> ExpandToSet;
typedef BstSet< int, CmpOrd<int> > IntSet;
typedef CmpTable< int, CmpOrd<int> > CmpIntSet;

typedef BstSet< long, CmpOrd<long> > LongSet;
typedef CmpTable< long, CmpOrd<long> > CmpLongSet;

typedef BstMap< long, long, CmpOrd<long> > LongMap;
typedef BstMapEl< long, long > LongMapEl;

typedef LongSet ProdIdSet;
typedef CmpLongSet CmpProdIdSet;

/* Set of states, list of states. */
typedef BstSet<PdaState*> PdaStateSet;
typedef Vector<PdaState*> StateVect;
typedef DList<PdaState> PdaStateList;

typedef LongMap FollowToAdd;
typedef LongMap ReductionMap;
typedef LongMapEl ReductionMapEl;

struct ProdIdPair
{
	ProdIdPair( int onReduce, int length )
		: onReduce(onReduce), length(length) {}

	int onReduce;
	int length;
};

struct CmpProdIdPair
{
	static inline int compare( const ProdIdPair &pair1, const ProdIdPair &pair2 )
	{ 
		if ( pair1.onReduce < pair2.onReduce )
			return -1;
		else if ( pair1.onReduce > pair2.onReduce )
			return 1;
		else if ( pair1.length < pair2.length )
			return -1;
		else if ( pair1.length > pair2.length )
			return 1;
		else
			return 0;
	}
};

typedef BstSet< ProdIdPair, CmpProdIdPair > ProdIdPairSet;

/* Transition class that implements actions and priorities. */
struct PdaTrans 
{
	PdaTrans() : 
		fromState(0), 
		toState(0), 
		isShift(false), 
		isShiftReduce(false),
		shiftPrior(0),
		noPreIgnore(false),
		noPostIgnore(false)
	{ }

	PdaTrans( const PdaTrans &other ) :
		lowKey(other.lowKey),
		fromState(0), toState(0),
		isShift(other.isShift),
		isShiftReduce(other.isShiftReduce),
		shiftPrior(other.shiftPrior),
		reductions(other.reductions),
		commits(other.commits),
		noPreIgnore(false),
		noPostIgnore(false)
	{ }

	long lowKey;
	PdaState *fromState;
	PdaState *toState;

	/* Pointers for outlist. */
	PdaTrans *prev, *next;

	/* Pointers for in-list. */
	PdaTrans *ilprev, *ilnext;

	long maxPrior();

	/* Parse Table construction data. */
	bool isShift, isShiftReduce;
	int shiftPrior;
	ReductionMap reductions;
	ActDataList actions;
	ActDataList actOrds;
	ActDataList actPriors;

	ExpandToSet expandTo;

	PdaActionSetEl *actionSetEl;

	LongSet commits;
	LongSet afterShiftCommits;

	bool noPreIgnore;
	bool noPostIgnore;
};

/* In transition list. Like DList except only has head pointers, which is all
 * that is required. Insertion and deletion is handled by the graph. This
 * class provides the iterator of a single list. */
struct PdaTransInList
{
	PdaTransInList() : head(0) { }

	PdaTrans *head;

	struct Iter
	{
		/* Default construct. */
		Iter() : ptr(0) { }

		/* Construct, assign from a list. */
		Iter( const PdaTransInList &il )  : ptr(il.head) { }
		Iter &operator=( const PdaTransInList &dl ) { ptr = dl.head; return *this; }

		/* At the end */
		bool lte() const    { return ptr != 0; }
		bool end() const    { return ptr == 0; }

		/* At the first, last element. */
		bool first() const { return ptr && ptr->ilprev == 0; }
		bool last() const  { return ptr && ptr->ilnext == 0; }

		/* Cast, dereference, arrow ops. */
		operator PdaTrans*() const   { return ptr; }
		PdaTrans &operator *() const { return *ptr; }
		PdaTrans *operator->() const { return ptr; }

		/* Increment, decrement. */
		inline void operator++(int)   { ptr = ptr->ilnext; }
		inline void operator--(int)   { ptr = ptr->ilprev; }

		/* The iterator is simply a pointer. */
		PdaTrans *ptr;
	};
};

typedef DList<PdaTrans> PdaTransList;

/* A element in a state dict. */
struct PdaStateDictEl 
:
	public AvlTreeEl<PdaStateDictEl>
{
	PdaStateDictEl(const PdaStateSet &stateSet) 
		: stateSet(stateSet) { }

	const PdaStateSet &getKey() { return stateSet; }
	PdaStateSet stateSet;
	PdaState *targState;
};

/* Dictionary mapping a set of states to a target state. */
typedef AvlTree< PdaStateDictEl, PdaStateSet, CmpTable<PdaState*> > PdaStateDict;

/* What items does a particular state encompass. */
typedef BstSet< long, CmpOrd<long> > DotSet;
typedef CmpTable< long, CmpOrd<long> > CmpDotSet;

/* Map of dot sets to states. */
typedef AvlTree< PdaState, DotSet, CmpDotSet > DotSetMap;
typedef PdaState DotSetMapEl;

typedef BstMap< long, PdaTrans* > TransMap;
typedef BstMapEl< long, PdaTrans* > TransMapEl;

/* State class that implements actions and priorities. */
struct PdaState 
: 
	public ClosureQueueListEl,
	public AvlTreeEl< PdaState >
{
	PdaState();
	PdaState(const PdaState &other);
	~PdaState();

	/* Is the state final? */
	bool isFinState() { return stateBits & SB_ISFINAL; }

	PdaTrans *findTrans( long key ) 
	{
		TransMapEl *transMapEl = transMap.find( key );
		if ( transMapEl == 0 )
			return 0;
		return transMapEl->value;
	}

	/* In transition list. */
	PdaTransInList inRange;

	ProdIdPairSet pendingCommits;

	/* When duplicating the fsm we need to map each 
	 * state to the new state representing it. */
	PdaState *stateMap;

	/* When merging states (state machine operations) this next pointer is
	 * used for the list of states that need to be filled in. */
	PdaState *alg_next;

	PdaStateSet *stateSet;

	/* Identification for printing and stable minimization. */
	int stateNum;

	/* A pointer to a dict element that contains the set of states this state
	 * represents. This cannot go into alg, because alg.next is used during
	 * the merging process. */
	PdaStateDictEl *stateDictEl;

	/* Bits controlling the behaviour of the state during collapsing to dfa. */
	int stateBits;

	/* State list elements. */
	PdaState *next, *prev;

	/* For dotset map. */
	DotSet &getKey() { return dotSet; }

	/* Closure management. */
	DotSet dotSet;
	DotSet dotSet2;
	bool onClosureQueue;
	bool inClosedMap;
	bool followMarked;
	bool onStateList;

	TransMap transMap;

	RegionVect regions;
	RegionVect preRegions;

	bool advanceReductions;
};

/* Compare lists of epsilon transitions. Entries are name ids of targets. */
typedef CmpTable< int, CmpOrd<int> > CmpEpsilonTrans;

/* Compare sets of context values. */
typedef CmpTable< int, CmpOrd<int> > CmpContextSets;

/* Graph class that implements actions and priorities. */
struct PdaGraph 
{
	/* Constructors/Destructors. */
	PdaGraph();
	PdaGraph( const PdaGraph &graph );
	~PdaGraph();

	/* The list of states. */
	PdaStateList stateList;
	PdaStateList misfitList;

	/* The start state. */
	PdaState *startState;
	PdaStateSet entryStateSet;

	/* The set of final states. */
	PdaStateSet finStateSet;

	/* Closure queues and maps. */
	DotSetMap closedMap;
	StateClosureQueue stateClosureQueue;
	StateClosureQueue stateClosedList;

	TransClosureQueue transClosureQueue;
	PdaState *stateClosureHead;

	LangEl **langElIndex;

	void setStartState( PdaState *state );
	void unsetStartState( );
	
	/*
	 * Basic attaching and detaching.
	 */

	/* Common to attaching/detaching list and default. */
	void attachToInList( PdaState *from, PdaState *to, PdaTrans *&head, PdaTrans *trans );
	void detachFromInList( PdaState *from, PdaState *to, PdaTrans *&head, PdaTrans *trans );

	/* Attach with a new transition. */
	PdaTrans *appendNewTrans( PdaState *from, PdaState *to, long onChar1, long );
	PdaTrans *insertNewTrans( PdaState *from, PdaState *to, long lowKey, long );

	/* Attach with an existing transition that already in an out list. */
	void attachTrans( PdaState *from, PdaState *to, PdaTrans *trans );
	
	/* Detach a transition from a target state. */
	void detachTrans( PdaState *from, PdaState *to, PdaTrans *trans );

	/* Detach a state from the graph. */
	void detachState( PdaState *state );

	/*
	 * Callbacks.
	 */

	/* Add in the properties of srcTrans into this. */
	void addInReduction( PdaTrans *dest, long prodId, long prior );
	void addInTrans( PdaTrans *destTrans, PdaTrans *srcTrans );
	void addInState( PdaState *destState, PdaState *srcState );

	/*
	 * Allocation.
	 */

	/* New up a state and add it to the graph. */
	PdaState *addState();

	/*
	 * Fsm operators.
	 */

	/* Follow to the fin state of src fsm. */
	PdaState *followFsm( PdaState *from, PdaGraph *srcFsm );

	/*
	 * Final states
	 */

	/* Set and Unset a state as final. */
	void setFinState( PdaState *state );
	void unsetFinState( PdaState *state );
	void unsetAllFinStates( );

	/* Set State numbers starting at 0. */
	void setStateNumbers();

	/*
	 * Path pruning
	 */

	/* Mark all states reachable from state. */
	void markReachableFromHere( PdaState *state );

	/* Removes states that cannot be reached by any path in the fsm and are
	 * thus wasted silicon. */
	void removeUnreachableStates();

	/* Remove error actions from states on which the error transition will
	 * never be taken. */
	bool outListCovers( PdaState *state );

	/* Remove states that are on the misfit list. */
	void removeMisfits();


	/*
	 * Other
	 */

	/* Move the in trans into src into dest. */
	void inTransMove(PdaState *dest, PdaState *src);

	int fsmLength( );

	/* Collected machine information. */
	unsigned long long maxState;
	unsigned long long maxAction;
	unsigned long long maxLelId;
	unsigned long long maxOffset;
	unsigned long long maxIndex;
	unsigned long long maxProdLen;

	PdaActionSet actionSet;
};


#endif /* _FSMGRAPH_H */
