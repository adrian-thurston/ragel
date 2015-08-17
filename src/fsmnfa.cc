/*
 *  Copyright 2015 Adrian Thurston <thurston@complang.org>
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

void FsmAp::nfaFillInStates()
{
	long count = nfaList.length();

	/* Can this lead to too many DFAs? Since the nfa merge is removing misfits,
	 * it is possible we remove a state that is on the nfa list, but we don't
	 * adjust count. */

	/* Merge any states that are awaiting merging. This will likey cause
	 * other states to be added to the stfil list. */
	while ( nfaList.length() > 0 && count-- ) {
		StateAp *state = nfaList.head;

		StateSet *stateSet = &state->stateDictEl->stateSet;
		nfaMergeStates( state, stateSet->data, stateSet->length() );

		for ( StateSet::Iter s = *stateSet; s.lte(); s++ )
			detachStateDict( state, *s );

		nfaList.detach( state );

		// std::cout << "misfit-list-len: " << misfitList.length() << std::endl;
	}
}



void FsmAp::prepareNfaRound()
{
	for ( StateList::Iter st = stateList; st.lte(); st++ ) {
		if ( st->nfaOut != 0 && ! (st->stateBits & STB_NFA_REP) ) {
			StateSet set;
			for ( NfaTransList::Iter to = *st->nfaOut; to.lte(); to++ )
				set.insert( to->toState );

			st->stateDictEl = new StateDictEl( set );
			st->stateDictEl->targState = st;
			stateDict.insert( st->stateDictEl );
			delete st->nfaOut;
			st->nfaOut = 0;

			nfaList.append( st );
		}
	}
}

void FsmAp::finalizeNfaRound()
{
	/* For any remaining NFA states, remove from the state dict. We need to
	 * keep the state sets. */
	for ( NfaStateList::Iter ns = nfaList; ns.lte(); ns++ )
		stateDict.detach( ns->stateDictEl );

	/* Disassociate non-nfa states from their state dicts. */
	for ( StateDict::Iter sdi = stateDict; sdi.lte(); sdi++ )
		sdi->targState->stateDictEl = 0;

	/* Delete the state dict elements for non-nfa states. */
	stateDict.empty();

	/* Transfer remaining stateDictEl sets to nfaOut. */
	while ( nfaList.length() > 0 ) {
		StateAp *state = nfaList.head;
		state->nfaOut = new NfaTransList;
		for ( StateSet::Iter ss = state->stateDictEl->stateSet; ss.lte(); ss++ ) {
			/* Attach it using the NFA transitions data structure (propigates
			 * to output). */
			NfaTrans *trans = new NfaTrans( 0, 0, 1 );
			state->nfaOut->append( trans );
			attachToNfa( state, *ss, trans );

			detachStateDict( state, *ss );
		}
		delete state->stateDictEl;
		state->stateDictEl = 0;
		nfaList.detach( state );
	}
}

void FsmAp::nfaMergeStates( StateAp *destState, 
		StateAp **srcStates, int numSrc )
{
	for ( int s = 0; s < numSrc; s++ ) {
		mergeStates( destState, srcStates[s] );

		while ( misfitList.length() > 0 ) {
			StateAp *state = misfitList.head;

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

void FsmAp::nfaRepeatOp1( Action *init, Action *min,
		Action *max, Action *push, Action *pop )
{
	/*
	 * First Concat.
	 */
	StateSet origFinals = finStateSet;

	/* Get the other's start state. */
	StateAp *origStartState = startState;
	StateAp *repStartState = dupStartState();

	StateAp *newStart = addState();

	newStart->nfaOut = new NfaTransList;

	NfaTrans *trans = new NfaTrans( push, pop, 1 );
	newStart->nfaOut->append( trans );
	attachToNfa( newStart, origStartState, trans );

	StateAp *newFinal = addState();

	for ( StateSet::Iter orig = origFinals; orig.lte(); orig++ ) {
		unsetFinState( *orig );

		StateAp *repl = addState();
		moveInwardTrans( repl, *orig );

		repl->nfaOut = new NfaTransList;

		NfaTrans *transOrig = new NfaTrans( push, pop, 3 );
		repl->nfaOut->append( transOrig );
		attachToNfa( repl, *orig, transOrig );

		NfaTrans *transRep = new NfaTrans( push, pop, 2 );
		repl->nfaOut->append( transRep );
		attachToNfa( repl, repStartState, transRep );

		NfaTrans *transNew = new NfaTrans( push, pop, 1 );
		repl->nfaOut->append( transNew );
		attachToNfa( repl, newFinal, transNew );
	}

	origStartState->fromStateActionTable.setAction( 0, init );
	newFinal->fromStateActionTable.setAction( 0, min );
	repStartState->fromStateActionTable.setAction( 0, max );

	unsetStartState();
	setStartState( newStart );
	setFinState( newFinal );
}

/* This version attempts to avoid the dup of the final states. It shaves some
 * time off but reduces correctness, haven't discovered why yet. */
void FsmAp::nfaRepeatOp1b( Action *init, Action *min,
		Action *max, Action *push, Action *pop )
{
	/*
	 * First Concat.
	 */
	StateSet origFinals = finStateSet;

	/* Get the other's start state. */
	StateAp *origStartState = startState;
	StateAp *repStartState = dupStartState();

	StateAp *newStart = addState();

	newStart->nfaOut = new NfaTransList;

	NfaTrans *trans = new NfaTrans( push, pop, 1 );
	newStart->nfaOut->append( trans );
	attachToNfa( newStart, origStartState, trans );

	StateAp *newFinal = addState();

	for ( StateSet::Iter orig = origFinals; orig.lte(); orig++ ) {
		StateAp *fin = *orig;
		unsetFinState( fin );
		if ( fin->nfaOut && fin->nfaOut->length() > 0 ) {
			StateAp *repl = addState();
			moveInwardTrans( repl, fin );
			
			repl->nfaOut = new NfaTransList;

			trans = new NfaTrans( push, pop, 1 );
			repl->nfaOut->append( trans );
			attachToNfa( fin, repStartState, trans );

			trans = new NfaTrans( push, pop, 2 );
			repl->nfaOut->append( trans );
			attachToNfa( fin, fin, trans );

			trans = new NfaTrans ( push, pop, 3 );
			repl->nfaOut->append( trans );
			attachToNfa( fin, newFinal, trans );
		}
		else {
			if ( fin->nfaOut == 0 )
				fin->nfaOut = new NfaTransList;

			trans = new NfaTrans( push, pop, 1 );
			fin->nfaOut->append( trans );
			attachToNfa( fin, repStartState, trans );

			trans = new NfaTrans( push, pop, 2 );
			fin->nfaOut->append( trans );
			attachToNfa( fin, newFinal, trans );
		}
	}

	origStartState->fromStateActionTable.setAction( 0, init );
	newFinal->fromStateActionTable.setAction( 0, min );
	repStartState->fromStateActionTable.setAction( 0, max );

	unsetStartState();
	setStartState( newStart );
	setFinState( newFinal );
}

/* This version uses conditions to test. This is better since it won't kill off
 * other paths with the fgoto error, but there are problems with ordering,
 * specifically if a repeat is repeated with something such as {2}. The init of
 * the second round executes before the condition test for the exit. This
 * implementation is suggesting we need to contain the repeat in some new kind
 * of NFA actions. */
void FsmAp::nfaRepeatOp2( Action *init, Action *inc, Action *min,
		Action *max, Action *push, Action *pop )
{
	/*
	 * First Concat.
	 */
	StateSet origFinals = finStateSet;

	/* Get the other's start state. */
	StateAp *origStartState = startState;
	StateAp *repStartState = dupStartState();

	StateAp *newStart = addState();

	newStart->nfaOut = new NfaTransList;

	NfaTrans *trans = new NfaTrans( push, pop, 1 );
	newStart->nfaOut->append( trans );
	attachToNfa( newStart, origStartState, trans );

	StateAp *newFinal = addState();

	for ( StateSet::Iter orig = origFinals; orig.lte(); orig++ ) {
		unsetFinState( *orig );

		StateAp *repl = addState();
		moveInwardTrans( repl, *orig );

		repl->nfaOut = new NfaTransList;

		trans = new NfaTrans( push, pop, 3 );
		repl->nfaOut->append( trans );
		attachToNfa( repl, *orig, trans );

		trans = new NfaTrans( push, pop, 2 );
		repl->nfaOut->append( trans );
		attachToNfa( repl, repStartState, trans );

		trans = new NfaTrans( push, pop, 1 );
		repl->nfaOut->append( trans );
		attachToNfa( repl, newFinal, trans );
	}

	origStartState->fromStateActionTable.setAction( 0, init );
	newFinal->fromStateActionTable.setAction( 1, inc );

	{
		addOutCondition( newFinal, min, true );
	}

	repStartState->fromStateActionTable.setAction( 1, inc );

	{
		CondSet set;
		CondKeySet vals;
		set.insert( max );
		vals.append( 1 );
		embedCondition( repStartState, set, vals );
	}

	unsetStartState();
	setStartState( newStart );
	setFinState( newFinal );
}

/* This version contains the init, increment and test in the nfa pop actions.
 * This is a compositional operator since it doesn't leave any actions to
 * trailing characters, where they may interact with other actions that use the
 * same variables. */
void FsmAp::nfaRepeatOp3( Action *push, Action *pop,
		Action *init, Action *stay, Action *repeat, Action *exit )
{
	/*
	 * First Concat.
	 */
	StateSet origFinals = finStateSet;

	/* Get the orig start state. */
	StateAp *origStartState = startState;
	StateAp *repStartState = dupStartState();

	/* New start state. */
	StateAp *newStart = addState();

	newStart->nfaOut = new NfaTransList;

	NfaTrans *trans = new NfaTrans( push, init, 1 );
	trans->popTest.setAction( 0, pop );
	newStart->nfaOut->append( trans );
	attachToNfa( newStart, origStartState, trans );

	StateAp *newFinal = addState();

	for ( StateSet::Iter orig = origFinals; orig.lte(); orig++ ) {
		unsetFinState( *orig );

		StateAp *repl = addState();
		moveInwardTrans( repl, *orig );

		repl->nfaOut = new NfaTransList;

		trans = new NfaTrans( push, stay, 3 );
		trans->popTest.setAction( 0, pop );
		repl->nfaOut->append( trans );
		attachToNfa( repl, *orig, trans );

		trans = new NfaTrans( push, repeat, 2 );
		trans->popTest.setAction( 0, pop );
		repl->nfaOut->append( trans );
		attachToNfa( repl, repStartState, trans );

		trans = new NfaTrans( push, exit, 1 );
		trans->popTest.setAction( 0, pop );
		repl->nfaOut->append( trans );
		attachToNfa( repl, newFinal, trans );
	}

	unsetStartState();
	setStartState( newStart );
	setFinState( newFinal );
}

void FsmAp::nfaGuard()
{
	StateAp *origStartState = startState;

	StateAp *newStart = addState();
	newStart->nfaOut = new NfaTransList;

	NfaTrans *trans = new NfaTrans( 0, 0, 1 );
	newStart->nfaOut->append( trans );
	attachToNfa( newStart, origStartState, trans );

	unsetStartState();
	setStartState( newStart );
}


/* Unions other with this machine. Other is deleted. */
void FsmAp::nfaUnionOp( FsmAp **others, int n, int depth )
{
	/* Mark existing NFA states as NFA_REP states, which excludes them from the
	 * prepare NFA round. We must treat them as final NFA states and not try to
	 * make them deterministic. */
	for ( StateList::Iter st = stateList; st.lte(); st++ ) {
		if ( st->nfaOut != 0 )
			st->stateBits |= STB_NFA_REP;
	}

	for ( int o = 0; o < n; o++ ) {
		for ( StateList::Iter st = others[o]->stateList; st.lte(); st++ ) {
			if ( st->nfaOut != 0 )
				st->stateBits |= STB_NFA_REP;
		}
	}

	for ( int o = 0; o < n; o++ )
		assert( ctx == others[o]->ctx );

	/* Not doing misfit accounting here. If we wanted to, it would need to be
	 * made nfa-compatibile. */

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

	if ( depth == 0 ) {
		startState->stateDictEl = new StateDictEl( startStateSet );
		nfaList.append( startState );

		for ( StateSet::Iter s = startStateSet; s.lte(); s++ ) {
			NfaTrans *trans = new NfaTrans( 0, 0, 0 );

			if ( startState->nfaOut == 0 )
				startState->nfaOut = new NfaTransList;

			startState->nfaOut->append( trans );
			attachToNfa( startState, *s, trans );
		}
	}
	else {
		/* Merge the start states. */
		if ( ctx->printStatistics )
			cout << "nfa-fill-round\t0" << endl;

		nfaMergeStates( startState, startStateSet.data, startStateSet.length() );

		long removed = removeUnreachableStates();
		if ( ctx->printStatistics )
			cout << "round-unreach\t" << removed << endl;

		/* Fill in any new states made from merging. */
		for ( long i = 1; i < depth; i++ ) {
			if ( ctx->printStatistics )
				cout << "nfa-fill-round\t" << i << endl;

			if ( nfaList.length() == 0 )
				break;

			nfaFillInStates( );

			long removed = removeUnreachableStates();
			if ( ctx->printStatistics )
				cout << "round-unreach\t" << removed << endl;
		}

		finalizeNfaRound();

		long maxStateSetSize = 0;
		long count = 0;
		for ( StateList::Iter s = stateList; s.lte(); s++ ) {
			if ( s->nfaOut != 0 && s->nfaOut->length() > 0 ) {
				count += 1;
				if ( s->nfaOut->length() > maxStateSetSize )
					maxStateSetSize = s->nfaOut->length();
			}
		}

		if ( ctx->printStatistics ) {
			cout << "fill-list\t" << count << endl;
			cout << "state-dict\t" << stateDict.length() << endl;
			cout << "states\t" << stateList.length() << endl;
			cout << "max-ss\t" << maxStateSetSize << endl;
		}

		removeUnreachableStates();

		if ( ctx->printStatistics )
			cout << "post-unreachable\t" << stateList.length() << endl;

		minimizePartition2();

		if ( ctx->printStatistics ) {
			std::cout << "post-min\t" << stateList.length() << std::endl;
			std::cout << std::endl;
		}
	}
}

