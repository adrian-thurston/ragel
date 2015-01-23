/*
 *  Copyright 2005-2012 Adrian Thurston <thurston@complang.org>
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
#include "compiler.h"

#include "vector.h"
#include <assert.h>
#include <string.h>
#include <iostream>

using std::endl;
using std::cerr;

void Compiler::lr0BringInItem( PdaGraph *pdaGraph, PdaState *dest, PdaState *prodState, 
		PdaTrans *expandFrom, Production *prod )
{
	/* We use dot sets for finding unique states. In the future, should merge
	 * dots sets with the stateSet pointer (only need one of these). */
	assert( dest != prodState );
	dest->dotSet.insert( prodState->dotSet );

	/* Get the epsilons, context, out priorities. */
	dest->pendingCommits.insert( prodState->pendingCommits );
	//if ( prodState->pendingCommits.length() > 0 )
	//	cerr << "THERE ARE PENDING COMMITS DRAWN IN" << endl;
	
	if ( prodState->transMap.length() > 0 ) {
		assert( prodState->transMap.length() == 1 );
		PdaTrans *srcTrans = prodState->transMap[0].value;

		/* Look for the source in the destination. */
		TransMapEl *destTel = dest->transMap.find( srcTrans->lowKey );
		if ( destTel == 0 ) {
			/* Make a new state and transition to it. */
			PdaState *newState = pdaGraph->addState();
			PdaTrans *newTrans = new PdaTrans();

			/* Attach the new transition to the new state. */
			newTrans->lowKey = srcTrans->lowKey;
			pdaGraph->attachTrans( dest, newState, newTrans );
			pdaGraph->addInTrans( newTrans, srcTrans );

			/* The transitions we make during lr0 closure are all shifts. */
			assert( newTrans->isShift );
			assert( srcTrans->isShift );

			/* The new state must have its state set setup. */
			newState->stateSet = new PdaStateSet;
			newState->stateSet->insert( srcTrans->toState );

			/* Insert the transition into the map. Be sure to set destTel, it
			 * is needed below. */
			dest->transMap.insert( srcTrans->lowKey, newTrans, &destTel );

			/* If the item is a non-term, queue it for closure. */
			LangEl *langEl = langElIndex[srcTrans->lowKey];
			if ( langEl != 0 && langEl->type == LangEl::NonTerm ) {
				pdaGraph->transClosureQueue.append( newTrans );
				//cerr << "put to trans closure queue" << endl;
			}
		}
		else {
			//cerr << "merging transitions" << endl;
			destTel->value->toState->stateSet->insert( srcTrans->toState );
			pdaGraph->addInTrans( destTel->value, srcTrans );
		}

		/* If this is an expansion then we may need to bring in commits. */
		if ( expandFrom != 0 && expandFrom->commits.length() > 0 ) {
			//cerr << "SETTING COMMIT ON CLOSURE ROUND" << endl;
			destTel->value->commits.insert( expandFrom->commits );

			expandFrom->commits.empty();
		}
	}
	else {
		/* ProdState does not have any transitions out. It is at the end of a
		 * production. */
		if ( expandFrom != 0 && expandFrom->commits.length() > 0 ) {
			//cerr << "SETTING COMMIT IN PENDING LOOKAHEAD" << endl;
			for ( LongSet::Iter len = expandFrom->commits; len.lte(); len++ )
				dest->pendingCommits.insert( ProdIdPair( prod->prodId, *len ) );

			expandFrom->commits.empty();
		}
	}
}

void Compiler::lr0InvokeClosure( PdaGraph *pdaGraph, PdaState *state )
{
	/* State should not already be closed. */
	assert( !state->inClosedMap );

	/* This is used each time we invoke closure, it must be cleared. */
	pdaGraph->transClosureQueue.abandon();

	/* Drag in the core items. */
	for ( PdaStateSet::Iter ssi = *state->stateSet; ssi.lte(); ssi++ )
		lr0BringInItem( pdaGraph, state, *ssi, 0, 0 );

	/* Now bring in the derived items. */
	while ( pdaGraph->transClosureQueue.length() > 0 ) {
		PdaTrans *toClose = pdaGraph->transClosureQueue.detachFirst();
		//cerr << "have a transition to derive" << endl;

		/* Get the langEl. */
		LangEl *langEl = langElIndex[toClose->lowKey];

		/* Make graphs for all of the productions that the non
		 * terminal goes to that are not already in the state's dotSet. */
		for ( LelDefList::Iter prod = langEl->defList; prod.lte(); prod++ ) {
			/* Bring in the start state of the production. */
			lr0BringInItem( pdaGraph, state, prod->fsm->startState, toClose, prod );
		}
	}

	/* Try and insert into the closed dict. */
	DotSetMapEl *lastFound;
	if ( pdaGraph->closedMap.insert( state, &lastFound ) ) {
		/* Insertion into closed dict succeeded. There is no state with the
		 * same dot set. The state is now closed. It is guaranteed a spot in
		 * the closed dict and it will never go away (states never deleted
		 * during closure). */
		pdaGraph->stateClosedList.append( state );
		state->inClosedMap = true;

		/* Add all of the states in the out transitions to the closure queue.
		 * This will give us a depth first search of the graph. */
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			/* Get the state the transEl goes to. */
			PdaState *targ = trans->value->toState;

			/* If the state on this tranisition has not already been slated
			 * for closure, then add it to the queue. */
			if ( !targ->onClosureQueue && !targ->inClosedMap ) {
				pdaGraph->stateClosureQueue.append( targ );
				targ->onClosureQueue = true;
			}
		}
	}
	else {
		/* Insertion into closed dict failed. There is an existing state
		 * with the same dot set. Get the existing state. */
		pdaGraph->inTransMove( lastFound, state );
		for ( TransMap::Iter tel = state->transMap; tel.lte(); tel++ ) {
			pdaGraph->stateList.detach( tel->value->toState );
			delete tel->value->toState;
			delete tel->value;
		}
		pdaGraph->stateList.detach( state );
		delete state;
	}
}

/* Invoke cloure on the graph. We use a queue here to achieve a breadth
 * first search of the tree we build. Note, there are back edges in this
 * tree. They are the edges made when upon closure, a dot set exists
 * already. */
void Compiler::lr0CloseAllStates( PdaGraph *pdaGraph )
{
	/* While there are items on the closure queue. */
	while ( pdaGraph->stateClosureQueue.length() > 0 ) {
		/* Pop the first item off. */
		PdaState *state = pdaGraph->stateClosureQueue.detachFirst();
		state->onClosureQueue = false;

		/* Invoke closure upon the state. */
		lr0InvokeClosure( pdaGraph, state );
	}
}

void Compiler::transferCommits( PdaGraph *pdaGraph, PdaTrans *trans, 
		PdaState *state, long prodId )
{
	ProdIdPairSet &pendingCommits = state->pendingCommits;
	for ( ProdIdPairSet::Iter pi = pendingCommits; pi.lte(); pi++ ) {
		if ( pi->onReduce == prodId )
			trans->commits.insert( pi->length );
	}
}

void Compiler::lalr1AddFollow2( PdaGraph *pdaGraph, PdaTrans *trans, FollowToAdd &followKeys )
{
	for ( ExpandToSet::Iter ets = trans->expandTo; ets.lte(); ets++ ) {
		int prodId = ets->prodId;
		PdaState *expandTo = ets->state;

		for ( FollowToAdd::Iter fkey = followKeys; fkey.lte(); fkey++ ) {
			TransMapEl *transEl = expandTo->transMap.find( fkey->key );

			if ( transEl != 0 ) {
				/* Set up the follow transition. */
				PdaTrans *destTrans = transEl->value;

				transferCommits( pdaGraph, destTrans, expandTo, prodId );

				pdaGraph->addInReduction( destTrans, prodId, fkey->value );
			}
			else {
				/* Set up the follow transition. */
				PdaTrans *followTrans = new PdaTrans;
				followTrans->lowKey = fkey->key;
				followTrans->isShift = false;
				followTrans->reductions.insert( prodId, fkey->value );

				transferCommits( pdaGraph, followTrans, expandTo, prodId );

				pdaGraph->attachTrans( expandTo, actionDestState, followTrans );
				expandTo->transMap.insert( followTrans->lowKey, followTrans );
				pdaGraph->transClosureQueue.append( followTrans );
			}
		}
	}
}

long PdaTrans::maxPrior()
{
	long prior = LONG_MIN;
	if ( isShift && shiftPrior > prior )
		prior = shiftPrior;
	for ( ReductionMap::Iter red = reductions; red.lte(); red++ ) {
		if ( red->value > prior )
			prior = red->value;
	}
	return prior;
}

void Compiler::lalr1AddFollow1( PdaGraph *pdaGraph, PdaState *state )
{
	/* Finding non-terminals into the state. */
	for ( PdaTransInList::Iter in = state->inRange; in.lte(); in++ ) {
		long key = in->lowKey; 
		LangEl *langEl = langElIndex[key];
		if ( langEl != 0 && langEl->type == LangEl::NonTerm ) {
			/* Finding the following transitions. */
			FollowToAdd followKeys;
			for ( TransMap::Iter fout = state->transMap; fout.lte(); fout++ ) {
				int fkey = fout->key; 
				LangEl *flel = langElIndex[fkey];
				if ( flel == 0 || flel->type == LangEl::Term ) {
					long prior = fout->value->maxPrior();
					followKeys.insert( fkey, prior );
				}
			}

			if ( followKeys.length() > 0 )
				lalr1AddFollow2( pdaGraph, in, followKeys );
		}
	}
}

void Compiler::lalr1AddFollow2( PdaGraph *pdaGraph, PdaTrans *trans, 
		long followKey, long prior )
{
	for ( ExpandToSet::Iter ets = trans->expandTo; ets.lte(); ets++ ) {
		int prodId = ets->prodId;
		PdaState *expandTo = ets->state;

		TransMapEl *transEl = expandTo->transMap.find( followKey );
		if ( transEl != 0 ) {
			/* Add in the reductions, or in the shift. */
			PdaTrans *destTrans = transEl->value;

			transferCommits( pdaGraph, destTrans, expandTo, prodId );

			pdaGraph->addInReduction( destTrans, prodId, prior );
		}
		else {
			/* Set up the follow transition. */
			PdaTrans *followTrans = new PdaTrans;
			followTrans->lowKey = followKey;
			followTrans->isShift = false;
			followTrans->reductions.insert( prodId, prior );

			transferCommits( pdaGraph, followTrans, expandTo, prodId );

			pdaGraph->attachTrans( expandTo, actionDestState, followTrans );
			expandTo->transMap.insert( followTrans->lowKey, followTrans );
			pdaGraph->transClosureQueue.append( followTrans );
		}
	}
}

void Compiler::lalr1AddFollow1( PdaGraph *pdaGraph, PdaTrans *trans )
{
	PdaState *state = trans->fromState;
	int fkey = trans->lowKey; 
	LangEl *flel = langElIndex[fkey];
	if ( flel == 0 || flel->type == LangEl::Term ) {
		/* Finding non-terminals into the state. */
		for ( PdaTransInList::Iter in = state->inRange; in.lte(); in++ ) {
			long key = in->lowKey; 
			LangEl *langEl = langElIndex[key];
			if ( langEl != 0 && langEl->type == LangEl::NonTerm ) {
				//cerr << "FOLLOW PRIOR TRANSFER 2: " << prior << endl;
				long prior = trans->maxPrior();
				lalr1AddFollow2( pdaGraph, in, fkey, prior );
			}
		}
	}
}

/* Add follow sets to an LR(0) graph to make it LALR(1). */
void Compiler::lalr1AddFollowSets( PdaGraph *pdaGraph, LangElSet &parserEls )
{
	/* Make the state that all reduction actions go to. Since a reduction pops
	 * states of the stack and sets the new target state, this state is
	 * actually never reached. Just here to link the trans to. */
	actionDestState = pdaGraph->addState();
	pdaGraph->setFinState( actionDestState );

	for ( LangElSet::Iter pe = parserEls; pe.lte(); pe++ ) {
		/* Get the entry into the graph and traverse over start. */
		PdaState *overStart = pdaGraph->followFsm( (*pe)->startState, (*pe)->rootDef->fsm );

		/* Add _eof after the initial _start. */
		PdaTrans *eofTrans = pdaGraph->insertNewTrans( overStart, actionDestState, 
				(*pe)->eofLel->id, (*pe)->eofLel->id );
		eofTrans->isShift = true;
	}

	/* This was used during lr0 table construction. */
	pdaGraph->transClosureQueue.abandon();

	/* Need to pass over every state initially. */
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ )
		lalr1AddFollow1( pdaGraph, state );

	/* While the closure queue has items, pop them off and add follow
	 * characters. */
	while ( pdaGraph->transClosureQueue.length() > 0 ) {
		/* Pop the first item off and add Follow for it . */
		PdaTrans *trans = pdaGraph->transClosureQueue.detachFirst();
		lalr1AddFollow1( pdaGraph, trans );
	}
}

void Compiler::linkExpansions( PdaGraph *pdaGraph )
{
	pdaGraph->setStateNumbers();
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		/* Find transitions out on non terminals. */
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			long key = trans->key;
			LangEl *langEl = langElIndex[key];
			if ( langEl != 0 && langEl->type == LangEl::NonTerm ) {
				/* For each production that the non terminal expand to ... */
				for ( LelDefList::Iter prod = langEl->defList; prod.lte(); prod++ ) {
					/* Follow the production and add to the trans's expand to set. */
					PdaState *followRes = pdaGraph->followFsm( state, prod->fsm );

					//LangEl *lel = langElIndex[key];
					//cerr << state->stateNum << ", "; 
					//if ( lel != 0 )
					//	cerr << lel->data;
					//else
					//	cerr << (char)key;
					//cerr << " -> " << (*fto)->stateNum << " on " <<
					//		prod->data << " (fss = " << fin.pos() << ")" << endl;
					trans->value->expandTo.insert( ExpandToEl( followRes, prod->prodId ) );
				}
			}
		}
	}
}

/* Add terminal versions of all nonterminal transitions. */
void Compiler::addDupTerms( PdaGraph *pdaGraph )
{
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		PdaTransList newTranitions;
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			LangEl *lel = langElIndex[trans->value->lowKey];
			if ( lel->type == LangEl::NonTerm ) {
				PdaTrans *dupTrans = new PdaTrans;
				dupTrans->lowKey = lel->termDup->id;
				dupTrans->isShift = true;

				/* Save the target state in to state. In the next loop when we
				 * attach the transition we must clear this because the
				 * attaching code requires the transition to be unattached. */
				dupTrans->toState = trans->value->toState;
				newTranitions.append( dupTrans );

				/* Commit code used? */
				//transferCommits( pdaGraph, followTrans, expandTo, prodId );
			}
		}

		for ( PdaTrans *dup = newTranitions.head; dup != 0; ) {
			PdaTrans *next = dup->next;
			PdaState *toState = dup->toState;
			dup->toState = 0;
			pdaGraph->attachTrans( state, toState, dup );
			state->transMap.insert( dup->lowKey, dup );
			dup = next;
		}
	}
}

/* Generate a LALR(1) graph. */
void Compiler::lalr1GenerateParser( PdaGraph *pdaGraph, LangElSet &parserEls )
{
	/* Make the intial graph. */
	pdaGraph->langElIndex = langElIndex;

	for ( Vector<LangEl*>::Iter r = parserEls; r.lte(); r++ ) {
		/* Create the entry point. */
		PdaState *rs = pdaGraph->addState();
		pdaGraph->entryStateSet.insert( rs );

		/* State set of just one state. */
		rs->stateSet = new PdaStateSet;
		rs->stateSet->insert( (*r)->rootDef->fsm->startState );

		/* Queue the start state for closure. */
		rs->onClosureQueue = true;
		pdaGraph->stateClosureQueue.append( rs );

		(*r)->startState = rs;
	}

	/* Run the lr0 closure. */
	lr0CloseAllStates( pdaGraph );

	/* Add terminal versions of all nonterminal transitions. */
	addDupTerms( pdaGraph );

	/* Link production expansions to the place they expand to. */
	linkExpansions( pdaGraph );

	/* Walk the graph adding follow sets to the LR(0) graph. */
	lalr1AddFollowSets( pdaGraph, parserEls );

//	/* Set the commit on the final eof shift. */
//	PdaTrans *overStart = pdaGraph->startState->findTrans( rootEl->id );
//	PdaTrans *eofTrans = overStart->toState->findTrans( eofLangEl->id );
//	eofTrans->afterShiftCommits.insert( 2 );
}
