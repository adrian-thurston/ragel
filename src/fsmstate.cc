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

#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <iostream>

#include "fsmgraph.h"

using namespace std;

/* Construct a mark index for a specified number of states. Must new up
 * an array that is states^2 in size. */
MarkIndex::MarkIndex( int states ) : numStates(states)
{
	/* Total pairs is states^2. Actually only use half of these, but we allocate
	 * them all to make indexing into the array easier. */
	int total = states * states;

	/* New up chars so that individual DListEl constructors are
	 * not called. Zero out the mem manually. */
	array = new bool[total];
	memset( array, 0, sizeof(bool) * total );
}

/* Free the array used to store state pairs. */
MarkIndex::~MarkIndex()
{
	delete[] array;
}

/* Mark a pair of states. States are specified by their number. The
 * marked states are moved from the unmarked list to the marked list. */
void MarkIndex::markPair(int state1, int state2)
{
	int pos = ( state1 >= state2 ) ?
		( state1 * numStates ) + state2 :
		( state2 * numStates ) + state1;

	array[pos] = true;
}

/* Returns true if the pair of states are marked. Returns false otherwise.
 * Ordering of states given does not matter. */
bool MarkIndex::isPairMarked(int state1, int state2)
{
	int pos = ( state1 >= state2 ) ?
		( state1 * numStates ) + state2 :
		( state2 * numStates ) + state1;

	return array[pos];
}

/* Create a new fsm state. State has not out transitions or in transitions, not
 * out out transition data and not number. */
FsmState::FsmState()
:
	/* No out or in transitions. */
	outList(),
	inList(),

	/* No entry points, or epsilon trans. */
	entryIds(),
	epsilonTrans(),

	/* No transitions in from other states. */
	foreignInTrans(0),

	/* Only used during merging. Normally null. */
	stateDictEl(0),
	eptVect(0),

	/* No state identification bits. */
	stateBits(0),

	/* No Priority data. */
	outPriorTable(),

	/* No Action data. */
	toStateActionTable(),
	fromStateActionTable(),
	outActionTable(),
	outCondSet(),
	errActionTable(),
	eofActionTable(),

	eofTarget(0)
{
}

/* Copy everything except actual the transitions. That is left up to the
 * FsmGraph copy constructor. */
FsmState::FsmState(const FsmState &other)
:
	/* All lists are cleared. They will be filled in when the
	 * individual transitions are duplicated and attached. */
	outList(),
	inList(),

	/* Duplicate the entry id set and epsilon transitions. These
	 * are sets of integers and as such need no fixing. */
	entryIds(other.entryIds),
	epsilonTrans(other.epsilonTrans),

	/* No transitions in from other states. */
	foreignInTrans(0),

	/* This is only used during merging. Normally null. */
	stateDictEl(0),
	eptVect(0),

	/* Fsm state data. */
	stateBits(other.stateBits),

	/* Copy in priority data. */
	outPriorTable(other.outPriorTable),

	/* Copy in action data. */
	toStateActionTable(other.toStateActionTable),
	fromStateActionTable(other.fromStateActionTable),
	outActionTable(other.outActionTable),
	outCondSet(other.outCondSet),
	errActionTable(other.errActionTable),
	eofActionTable(other.eofActionTable),

	eofTarget(0)
{
	/* Duplicate all the transitions. */
	for ( TransList::Iter trans = other.outList; trans.lte(); trans++ ) {
		/* Dupicate and store the orginal target in the transition. This will
		 * be corrected once all the states have been created. */
		FsmTrans *newTrans = new FsmTrans(*trans);
		newTrans->toState = trans->toState;
		outList.append( newTrans );
	}
}

/* If there is a state dict element, then delete it. Everything else is left
 * up to the FsmGraph destructor. */
FsmState::~FsmState()
{
	if ( stateDictEl != 0 )
		delete stateDictEl;
}

/* Compare two states using pointers to the states. With the approximate
 * compare the idea is that if the compare finds them the same, they can
 * immediately be merged. */
int ApproxCompare::compare( const FsmState *state1 , const FsmState *state2 )
{
	int compareRes;

	/* Test final state status. */
	if ( (state1->stateBits & SB_ISFINAL) && !(state2->stateBits & SB_ISFINAL) )
		return -1;
	else if ( !(state1->stateBits & SB_ISFINAL) && (state2->stateBits & SB_ISFINAL) )
		return 1;
	
	/* Test epsilon transition sets. */
	compareRes = CmpEpsilonTrans::compare( state1->epsilonTrans, 
			state2->epsilonTrans );
	if ( compareRes != 0 )
		return compareRes;
	
	/* Compare the out transitions. */
	compareRes = FsmGraph::compareStateData( state1, state2 );
	if ( compareRes != 0 )
		return compareRes;

	/* Use a pair iterator to get the transition pairs. */
	PairIter<FsmTrans> outPair( state1->outList.head, state2->outList.head );
	for ( ; !outPair.end(); outPair++ ) {
		switch ( outPair.userState ) {

		case RangeInS1:
			compareRes = FsmGraph::compareFullPtr( outPair.s1Tel.trans, 0 );
			if ( compareRes != 0 )
				return compareRes;
			break;

		case RangeInS2:
			compareRes = FsmGraph::compareFullPtr( 0, outPair.s2Tel.trans );
			if ( compareRes != 0 )
				return compareRes;
			break;

		case RangeOverlap:
			compareRes = FsmGraph::compareFullPtr( 
					outPair.s1Tel.trans, outPair.s2Tel.trans );
			if ( compareRes != 0 )
				return compareRes;
			break;

		case BreakS1:
		case BreakS2:
			break;
		}
	}

	/* Got through the entire state comparison, deem them equal. */
	return 0;
}

/* Compare class for the sort that does the intial partition of compaction. */
int InitPartitionCompare::compare( const FsmState *state1 , const FsmState *state2 )
{
	int compareRes;

	/* Test final state status. */
	if ( (state1->stateBits & SB_ISFINAL) && !(state2->stateBits & SB_ISFINAL) )
		return -1;
	else if ( !(state1->stateBits & SB_ISFINAL) && (state2->stateBits & SB_ISFINAL) )
		return 1;

	/* Test epsilon transition sets. */
	compareRes = CmpEpsilonTrans::compare( state1->epsilonTrans, 
			state2->epsilonTrans );
	if ( compareRes != 0 )
		return compareRes;

	/* Compare the out transitions. */
	compareRes = FsmGraph::compareStateData( state1, state2 );
	if ( compareRes != 0 )
		return compareRes;

	/* Use a pair iterator to test the transition pairs. */
	PairIter<FsmTrans> outPair( state1->outList.head, state2->outList.head );
	for ( ; !outPair.end(); outPair++ ) {
		switch ( outPair.userState ) {

		case RangeInS1:
			compareRes = FsmGraph::compareDataPtr( outPair.s1Tel.trans, 0 );
			if ( compareRes != 0 )
				return compareRes;
			break;

		case RangeInS2:
			compareRes = FsmGraph::compareDataPtr( 0, outPair.s2Tel.trans );
			if ( compareRes != 0 )
				return compareRes;
			break;

		case RangeOverlap:
			compareRes = FsmGraph::compareDataPtr( 
					outPair.s1Tel.trans, outPair.s2Tel.trans );
			if ( compareRes != 0 )
				return compareRes;
			break;

		case BreakS1:
		case BreakS2:
			break;
		}
	}

	return 0;
}

/* Compare class for the sort that does the partitioning. */
int PartitionCompare::compare( const FsmState *state1, const FsmState *state2 )
{
	int compareRes;

	/* Use a pair iterator to get the transition pairs. */
	PairIter<FsmTrans> outPair( state1->outList.head, state2->outList.head );
	for ( ; !outPair.end(); outPair++ ) {
		switch ( outPair.userState ) {

		case RangeInS1:
			compareRes = FsmGraph::comparePartPtr( outPair.s1Tel.trans, 0 );
			if ( compareRes != 0 )
				return compareRes;
			break;

		case RangeInS2:
			compareRes = FsmGraph::comparePartPtr( 0, outPair.s2Tel.trans );
			if ( compareRes != 0 )
				return compareRes;
			break;

		case RangeOverlap:
			compareRes = FsmGraph::comparePartPtr( 
					outPair.s1Tel.trans, outPair.s2Tel.trans );
			if ( compareRes != 0 )
				return compareRes;
			break;

		case BreakS1:
		case BreakS2:
			break;
		}
	}

	return 0;
}

/* Compare class for the sort that does the partitioning. */
bool MarkCompare::shouldMark( MarkIndex &markIndex, const FsmState *state1, 
			const FsmState *state2 )
{
	/* Use a pair iterator to get the transition pairs. */
	PairIter<FsmTrans> outPair( state1->outList.head, state2->outList.head );
	for ( ; !outPair.end(); outPair++ ) {
		switch ( outPair.userState ) {

		case RangeInS1:
			if ( FsmGraph::shouldMarkPtr( markIndex, outPair.s1Tel.trans, 0 ) )
				return true;
			break;

		case RangeInS2:
			if ( FsmGraph::shouldMarkPtr( markIndex, 0, outPair.s2Tel.trans ) )
				return true;
			break;

		case RangeOverlap:
			if ( FsmGraph::shouldMarkPtr( markIndex,
					outPair.s1Tel.trans, outPair.s2Tel.trans ) )
				return true;
			break;

		case BreakS1:
		case BreakS2:
			break;
		}
	}

	return false;
}

/*
 * Transition Comparison.
 */

/* Compare target partitions. Either pointer may be null. */
int FsmGraph::comparePartPtr( FsmTrans *trans1, FsmTrans *trans2 )
{
	if ( trans1 != 0 ) {
		/* If trans1 is set then so should trans2. The initial partitioning
		 * guarantees this for us. */
		if ( trans1->toState == 0 && trans2->toState != 0 )
			return -1;
		else if ( trans1->toState != 0 && trans2->toState == 0 )
			return 1;
		else if ( trans1->toState != 0 ) {
			/* Both of targets are set. */
			return CmpOrd< MinPartition* >::compare( 
				trans1->toState->alg.partition, trans2->toState->alg.partition );
		}
	}
	return 0;
}


/* Compares two transition pointers according to priority and functions.
 * Either pointer may be null. Does not consider to state or from state. */
int FsmGraph::compareDataPtr( FsmTrans *trans1, FsmTrans *trans2 )
{
	if ( trans1 == 0 && trans2 != 0 )
		return -1;
	else if ( trans1 != 0 && trans2 == 0 )
		return 1;
	else if ( trans1 != 0 ) {
		/* Both of the transition pointers are set. */
		int compareRes = compareTransData( trans1, trans2 );
		if ( compareRes != 0 )
			return compareRes;
	}
	return 0;
}

/* Compares two transitions according to target state, priority and functions.
 * Does not consider from state. Either of the pointers may be null. */
int FsmGraph::compareFullPtr( FsmTrans *trans1, FsmTrans *trans2 )
{
	if ( (trans1 != 0) ^ (trans2 != 0) ) {
		/* Exactly one of the transitions is set. */
		if ( trans1 != 0 )
			return -1;
		else
			return 1;
	}
	else if ( trans1 != 0 ) {
		/* Both of the transition pointers are set. Test target state,
		 * priority and funcs. */
		if ( trans1->toState < trans2->toState )
			return -1;
		else if ( trans1->toState > trans2->toState )
			return 1;
		else if ( trans1->toState != 0 ) {
			/* Test transition data. */
			int compareRes = compareTransData( trans1, trans2 );
			if ( compareRes != 0 )
				return compareRes;
		}
	}
	return 0;
}


bool FsmGraph::shouldMarkPtr( MarkIndex &markIndex, FsmTrans *trans1, 
				FsmTrans *trans2 )
{
	if ( (trans1 != 0) ^ (trans2 != 0) ) {
		/* Exactly one of the transitions is set. The initial mark round
		 * should rule out this case. */
		assert( false );
	}
	else if ( trans1 != 0 ) {
		/* Both of the transitions are set. If the target pair is marked, then
		 * the pair we are considering gets marked. */
		return markIndex.isPairMarked( trans1->toState->alg.stateNum, 
				trans2->toState->alg.stateNum );
	}

	/* Neither of the transitiosn are set. */
	return false;
}


