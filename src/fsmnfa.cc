/*
 *  Copyright 2015, 2016 Adrian Thurston <thurston@complang.org>
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
			NfaTrans *trans = new NfaTrans( /* 0, 0, */ 1 );
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
	}
}


/*
 * WRT action ordering.
 *
 * All the pop restore actions get an ordering of -2 to cause them to always
 * execute first. This is the action that restores the state and we need that
 * to happen before any user actions. 
 */
const int ORD_PUSH = 0;
const int ORD_RESTORE = -2;
const int ORD_COND = -1;
const int ORD_TEST = 1073741824;

void FsmAp::transferOutToNfaTrans( NfaTrans *trans, StateAp *state )
{
	trans->popAction.setActions( state->outActionTable );
	trans->popCondSpace = state->outCondSpace;
	trans->popCondKeys = state->outCondKeys;
	trans->priorTable.setPriors( state->outPriorTable );
}

/* This version contains the init, increment and test in the nfa pop actions.
 * This is a compositional operator since it doesn't leave any actions to
 * trailing characters, where they may interact with other actions that use the
 * same variables. */
FsmRes FsmAp::nfaRepeatOp( FsmAp *fsm, Action *push, Action *pop, Action *init,
		Action *stay, Action *repeat, Action *exit )
{
	/*
	 * First Concat.
	 */
	StateSet origFinals = fsm->finStateSet;

	/* Get the orig start state. */
	StateAp *origStartState = fsm->startState;
	StateAp *repStartState = fsm->dupStartState();

	/* New start state. */
	StateAp *newStart = fsm->addState();

	newStart->nfaOut = new NfaTransList;

	NfaTrans *trans;
	if ( init ) {
		/* Transition into the repetition. Doesn't make much sense to flip this
		 * statically false, but provided for consistency of interface. */
		trans = new NfaTrans( 1 );

		trans->pushTable.setAction( ORD_PUSH, push );
		trans->restoreTable.setAction( ORD_RESTORE, pop );
		trans->popTest.setAction( ORD_TEST, init );

		newStart->nfaOut->append( trans );
		fsm->attachToNfa( newStart, origStartState, trans );
	}

	StateAp *newFinal = fsm->addState();

	for ( StateSet::Iter orig = origFinals; orig.lte(); orig++ ) {
		/* For every final state, we place a new final state in front of it,
		 * with an NFA transition to the original. This is the "stay" choice. */
		StateAp *repl = fsm->addState();
		fsm->moveInwardTrans( repl, *orig );

		repl->nfaOut = new NfaTransList;

		if ( stay != 0 ) {
			/* Transition to original final state. Represents staying. */
			trans = new NfaTrans( 3 );

			trans->pushTable.setAction( ORD_PUSH, push );
			trans->restoreTable.setAction( ORD_RESTORE, pop );
			trans->popTest.setAction( ORD_TEST, stay );

			repl->nfaOut->append( trans );
			fsm->attachToNfa( repl, *orig, trans );
		}

		/* Transition back to the start. Represents repeat. */
		if ( repeat != 0 ) {
			trans = new NfaTrans( 2 );

			trans->pushTable.setAction( ORD_PUSH, push );
			trans->restoreTable.setAction( ORD_RESTORE, pop );
			trans->popTest.setAction( ORD_TEST, repeat );

			fsm->transferOutToNfaTrans( trans, *orig );

			repl->nfaOut->append( trans );
			fsm->attachToNfa( repl, repStartState, trans );
		}

		if ( exit != 0 ) {
			/* Transition to thew new final. Represents exiting. */
			trans = new NfaTrans( 1 );

			trans->pushTable.setAction( ORD_PUSH, push );
			trans->restoreTable.setAction( ORD_RESTORE, pop );
			trans->popTest.setAction( ORD_TEST, exit );

			fsm->transferOutToNfaTrans( trans, *orig );

			repl->nfaOut->append( trans );
			fsm->attachToNfa( repl, newFinal, trans );
		}

		fsm->unsetFinState( *orig );
	}

	fsm->unsetStartState();
	fsm->setStartState( newStart );
	fsm->setFinState( newFinal );

	return FsmRes( FsmRes::Fsm(), fsm );
}


/* Unions others with fsm. Others are deleted. */
FsmRes FsmAp::nfaUnionOp( FsmAp *fsm, FsmAp **others, int n, int depth, ostream &stats )
{
	/* Mark existing NFA states as NFA_REP states, which excludes them from the
	 * prepare NFA round. We must treat them as final NFA states and not try to
	 * make them deterministic. */
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
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
		assert( fsm->ctx == others[o]->ctx );

	/* Not doing misfit accounting here. If we wanted to, it would need to be
	 * made nfa-compatibile. */

	/* Build a state set consisting of both start states */
	StateSet startStateSet;
	startStateSet.insert( fsm->startState );
	for ( int o = 0; o < n; o++ )
		startStateSet.insert( others[o]->startState );

	/* Both of the original start states loose their start state status. */
	fsm->unsetStartState();
	for ( int o = 0; o < n; o++ )
		others[o]->unsetStartState();

	/* Bring in the rest of other's entry points. */
	for ( int o = 0; o < n; o++ ) {
		fsm->copyInEntryPoints( others[o] );
		others[o]->entryPoints.empty();
	}

	for ( int o = 0; o < n; o++ ) {
		/* Merge the lists. This will move all the states from other
		 * into this. No states will be deleted. */
		fsm->stateList.append( others[o]->stateList );
		fsm->misfitList.append( others[o]->misfitList );
		// nfaList.append( others[o]->nfaList );
	}

	for ( int o = 0; o < n; o++ ) {
		/* Move the final set data from other into this. */
		fsm->finStateSet.insert( others[o]->finStateSet );
		others[o]->finStateSet.empty();
	}

	for ( int o = 0; o < n; o++ ) {
		/* Since other's list is empty, we can delete the fsm without
		 * affecting any states. */
		delete others[o];
	}

	/* Create a new start state. */
	fsm->setStartState( fsm->addState() );

	if ( depth == 0 ) {
		fsm->startState->stateDictEl = new StateDictEl( startStateSet );
		fsm->nfaList.append( fsm->startState );

		for ( StateSet::Iter s = startStateSet; s.lte(); s++ ) {
			NfaTrans *trans = new NfaTrans( /* 0, 0, */ 0 );

			if ( fsm->startState->nfaOut == 0 )
				fsm->startState->nfaOut = new NfaTransList;

			fsm->startState->nfaOut->append( trans );
			fsm->attachToNfa( fsm->startState, *s, trans );
		}
	}
	else {
		/* Merge the start states. */
		if ( fsm->ctx->printStatistics )
			stats << "nfa-fill-round\t0" << endl;

		fsm->nfaMergeStates( fsm->startState, startStateSet.data, startStateSet.length() );

		long removed = fsm->removeUnreachableStates();
		if ( fsm->ctx->printStatistics )
			stats << "round-unreach\t" << removed << endl;

		/* Fill in any new states made from merging. */
		for ( long i = 1; i < depth; i++ ) {
			if ( fsm->ctx->printStatistics )
				stats << "nfa-fill-round\t" << i << endl;

			if ( fsm->nfaList.length() == 0 )
				break;

			fsm->nfaFillInStates( );

			long removed = fsm->removeUnreachableStates();
			if ( fsm->ctx->printStatistics )
				stats << "round-unreach\t" << removed << endl;
		}

		fsm->finalizeNfaRound();

		long maxStateSetSize = 0;
		long count = 0;
		for ( StateList::Iter s = fsm->stateList; s.lte(); s++ ) {
			if ( s->nfaOut != 0 && s->nfaOut->length() > 0 ) {
				count += 1;
				if ( s->nfaOut->length() > maxStateSetSize )
					maxStateSetSize = s->nfaOut->length();
			}
		}

		if ( fsm->ctx->printStatistics ) {
			stats << "fill-list\t" << count << endl;
			stats << "state-dict\t" << fsm->stateDict.length() << endl;
			stats << "states\t" << fsm->stateList.length() << endl;
			stats << "max-ss\t" << maxStateSetSize << endl;
		}

		fsm->removeUnreachableStates();

		if ( fsm->ctx->printStatistics )
			stats << "post-unreachable\t" << fsm->stateList.length() << endl;

		fsm->minimizePartition2();

		if ( fsm->ctx->printStatistics ) {
			stats << "post-min\t" << fsm->stateList.length() << std::endl;
			stats << std::endl;
		}
	}

	return FsmRes( FsmRes::Fsm(), fsm );
}

FsmRes FsmAp::nfaUnion( const NfaRoundVect &roundsList, FsmAp **machines, int numMachines, std::ostream &stats, bool printStatistics )
{
	long sumPlain = 0, sumMin = 0;
	for ( int i = 0; i < numMachines; i++ ) {
		sumPlain += machines[i]->stateList.length();

		machines[i]->removeUnreachableStates();
		machines[i]->minimizePartition2();

		sumMin += machines[i]->stateList.length();
	}

	if ( printStatistics ) {
		stats << "sum-plain\t" << sumPlain << endl;
		stats << "sum-minimized\t" << sumMin << endl;
	}

	/* For each round. */
	for ( NfaRoundVect::Iter r = roundsList; r.lte(); r++ ) {

		if ( printStatistics ) {
			stats << "depth\t" << r->depth << endl;
			stats << "grouping\t" << r->groups << endl;
		}

		int numGroups = 0;
		int start = 0;
		while ( start < numMachines ) {
			/* If nfa-group-max is zero, don't group, put all terms into a single
			 * n-depth NFA. */
			int amount = r->groups == 0 ? numMachines : r->groups;
			if ( ( start + amount ) > numMachines )
				amount = numMachines - start;

			FsmAp **others = machines + start + 1;
			FsmRes res = FsmAp::nfaUnionOp( machines[start], others, (amount - 1), r->depth, stats );
			machines[start] = res.fsm;

			start += amount;
			numGroups++;
		}

		if ( numGroups == 1 )
			break;

		/* Move the group starts into the groups array. */
		FsmAp **groups = new FsmAp*[numGroups];
		int g = 0;
		start = 0;
		while ( start < numMachines ) {
			groups[g] = machines[start];
			start += r->groups == 0 ? numMachines : r->groups;
			g++;
		}

		delete[] machines;
		machines = groups;
		numMachines = numGroups;
	}

	FsmAp *ret = machines[0];
	return FsmRes( FsmRes::Fsm(), ret );
}
