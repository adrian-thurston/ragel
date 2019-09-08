/*
 * Copyright 2001-2018 Adrian Thurston <thurston@colm.net>
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

#include <iostream>
#include <iomanip>
#include <sstream>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <inputdata.h>

/* Parsing. */
#include "ragel.h"
#include "parsetree.h"
#include "parsedata.h"

void LongestMatch::runLongestMatch( ParseData *pd, FsmAp *graph )
{
	graph->markReachableFromHereStopFinal( graph->startState );
	for ( StateList::Iter ms = graph->stateList; ms.lte(); ms++ ) {
		if ( ms->stateBits & STB_ISMARKED ) {
			ms->lmItemSet.insert( 0 );
			ms->stateBits &= ~ STB_ISMARKED;
		}
	}

	/* Transfer the first item of non-empty lmAction tables to the item sets
	 * of the states that follow. Exclude states that have no transitions out.
	 * This must happen on a separate pass so that on each iteration of the
	 * next pass we have the item set entries from all lmAction tables. */
	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
			if ( trans->plain() ) {
				TransDataAp *tdap = trans->tdap();
				if ( tdap->lmActionTable.length() > 0 ) {
					LmActionTableEl *lmAct = tdap->lmActionTable.data;
					StateAp *toState = tdap->toState;
					assert( toState );

					/* Can only optimize this if there are no transitions out.
					 * Note there can be out transitions going nowhere with
					 * actions and they too must inhibit this optimization. */
					if ( toState->outList.length() > 0 ) {
						/* Fill the item sets. */
						graph->markReachableFromHereStopFinal( toState );
						for ( StateList::Iter ms = graph->stateList; ms.lte(); ms++ ) {
							if ( ms->stateBits & STB_ISMARKED ) {
								ms->lmItemSet.insert( lmAct->value );
								ms->stateBits &= ~ STB_ISMARKED;
							}
						}
					}
				}
			}
			else {
				for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) {
					if ( cond->lmActionTable.length() > 0 ) {

						LmActionTableEl *lmAct = cond->lmActionTable.data;
						StateAp *toState = cond->toState;
						assert( toState );

						/* Can only optimize this if there are no transitions out.
						 * Note there can be out transitions going nowhere with
						 * actions and they too must inhibit this optimization. */
						if ( toState->outList.length() > 0 ) {
							/* Fill the item sets. */
							graph->markReachableFromHereStopFinal( toState );
							for ( StateList::Iter ms = graph->stateList; ms.lte(); ms++ ) {
								if ( ms->stateBits & STB_ISMARKED ) {
									ms->lmItemSet.insert( lmAct->value );
									ms->stateBits &= ~ STB_ISMARKED;
								}
							}
						}
					}
				}
			}
		}
	}

	/* The lmItem sets are now filled, telling us which longest match rules
	 * can succeed in which states. First determine if we need to make sure
	 * act is defaulted to zero. We need to do this if there are any states
	 * with lmItemSet.length() > 1 and NULL is included. That is, that the
	 * switch may get called when in fact nothing has been matched. */
	int maxItemSetLength = 0;
	graph->markReachableFromHereStopFinal( graph->startState );
	for ( StateList::Iter ms = graph->stateList; ms.lte(); ms++ ) {
		if ( ms->stateBits & STB_ISMARKED ) {
			if ( ms->lmItemSet.length() > maxItemSetLength )
				maxItemSetLength = ms->lmItemSet.length();
			ms->stateBits &= ~ STB_ISMARKED;
		}
	}

	/* The actions executed on starting to match a token. */
	FsmRes res = FsmAp::isolateStartState( graph );
	graph = res.fsm;
	graph->startState->toStateActionTable.setAction( pd->initTokStartOrd, pd->initTokStart );
	graph->startState->fromStateActionTable.setAction( pd->setTokStartOrd, pd->setTokStart );
	if ( maxItemSetLength > 1 ) {
		/* The longest match action switch may be called when tokens are
		 * matched, in which case act must be initialized, there must be a
		 * case to handle the error, and the generated machine will require an
		 * error state. */
		lmSwitchHandlesError = true;
		pd->fsmCtx->lmRequiresErrorState = true;
		graph->startState->toStateActionTable.setAction( pd->initActIdOrd, pd->initActId );
	}

	/* The place to store transitions to restart. It maybe possible for the
	 * restarting to affect the searching through the graph that follows. For
	 * now take the safe route and save the list of transitions to restart
	 * until after all searching is done. */
	Vector<TransAp*> restartData;
	Vector<CondAp*> restartCond;

	/* Set actions that do immediate token recognition, set the longest match part
	 * id and set the token ending. */
	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
			if ( trans->plain() ) {
				TransDataAp *tdap = trans->tdap();
				if ( tdap->lmActionTable.length() > 0 ) {
					LmActionTableEl *lmAct = tdap->lmActionTable.data;
					StateAp *toState = tdap->toState;
					assert( toState );

					/* Can only optimize this if there are no transitions out.
					 * Note there can be out transitions going nowhere with
					 * actions and they too must inhibit this optimization. */
					if ( toState->outList.length() == 0 ) {
						/* Can execute the immediate action for the longest match
						 * part. Redirect the action to the start state.
						 *
						 * NOTE: When we need to inhibit on_last due to leaving
						 * actions the above test suffices. If the state has out
						 * actions then it will fail because the out action will
						 * have been transferred to an error transition, which
						 * makes the outlist non-empty. */
						tdap->actionTable.setAction( lmAct->key, 
								lmAct->value->actOnLast );
						restartData.append( trans );
					}
					else {
						/* Look for non final states that have a non-empty item
						 * set. If these are present then we need to record the
						 * end of the token.  Also Find the highest item set
						 * length reachable from here (excluding at transtions to
						 * final states). */
						bool nonFinalNonEmptyItemSet = false;
						maxItemSetLength = 0;
						graph->markReachableFromHereStopFinal( toState );
						for ( StateList::Iter ms = graph->stateList; ms.lte(); ms++ ) {
							if ( ms->stateBits & STB_ISMARKED ) {
								if ( ms->lmItemSet.length() > 0 && !ms->isFinState() )
									nonFinalNonEmptyItemSet = true;
								if ( ms->lmItemSet.length() > maxItemSetLength )
									maxItemSetLength = ms->lmItemSet.length();
								ms->stateBits &= ~ STB_ISMARKED;
							}
						}

						/* If there are reachable states that are not final and
						 * have non empty item sets or that have an item set
						 * length greater than one then we need to set tokend
						 * because the error action that matches the token will
						 * require it. */
						if ( nonFinalNonEmptyItemSet || maxItemSetLength > 1 )
							tdap->actionTable.setAction( pd->setTokEndOrd, pd->setTokEnd );

						/* Some states may not know which longest match item to
						 * execute, must set it. */
						if ( maxItemSetLength > 1 ) {
							/* There are transitions out, another match may come. */
							tdap->actionTable.setAction( lmAct->key, 
									lmAct->value->setActId );
						}
					}
				}
			}
			else {
				for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) {
					if ( cond->lmActionTable.length() > 0 ) {
						LmActionTableEl *lmAct = cond->lmActionTable.data;
						StateAp *toState = cond->toState;
						assert( toState );

						/* Can only optimize this if there are no transitions out.
						 * Note there can be out transitions going nowhere with
						 * actions and they too must inhibit this optimization. */
						if ( toState->outList.length() == 0 ) {
							/* Can execute the immediate action for the longest match
							 * part. Redirect the action to the start state.
							 *
							 * NOTE: When we need to inhibit on_last due to leaving
							 * actions the above test suffices. If the state has out
							 * actions then it will fail because the out action will
							 * have been transferred to an error transition, which
							 * makes the outlist non-empty. */
							cond->actionTable.setAction( lmAct->key, 
									lmAct->value->actOnLast );
							restartCond.append( cond );
						}
						else {
							/* Look for non final states that have a non-empty item
							 * set. If these are present then we need to record the
							 * end of the token.  Also Find the highest item set
							 * length reachable from here (excluding at transtions to
							 * final states). */
							bool nonFinalNonEmptyItemSet = false;
							maxItemSetLength = 0;
							graph->markReachableFromHereStopFinal( toState );
							for ( StateList::Iter ms = graph->stateList; ms.lte(); ms++ ) {
								if ( ms->stateBits & STB_ISMARKED ) {
									if ( ms->lmItemSet.length() > 0 && !ms->isFinState() )
										nonFinalNonEmptyItemSet = true;
									if ( ms->lmItemSet.length() > maxItemSetLength )
										maxItemSetLength = ms->lmItemSet.length();
									ms->stateBits &= ~ STB_ISMARKED;
								}
							}

							/* If there are reachable states that are not final and
							 * have non empty item sets or that have an item set
							 * length greater than one then we need to set tokend
							 * because the error action that matches the token will
							 * require it. */
							if ( nonFinalNonEmptyItemSet || maxItemSetLength > 1 )
								cond->actionTable.setAction( pd->setTokEndOrd, pd->setTokEnd );

							/* Some states may not know which longest match item to
							 * execute, must set it. */
							if ( maxItemSetLength > 1 ) {
								/* There are transitions out, another match may come. */
								cond->actionTable.setAction( lmAct->key, lmAct->value->setActId );
							}
						}
					}
				}
			}
		}
	}

	/* Now that all graph searching is done it certainly safe set the
	 * restarting. It may be safe above, however this must be verified. */
	for ( Vector<TransAp*>::Iter pt = restartData; pt.lte(); pt++ )
		restart( graph, *pt );

	for ( Vector<CondAp*>::Iter pt = restartCond; pt.lte(); pt++ )
		restart( graph, *pt );

	int lmErrActionOrd = pd->fsmCtx->curActionOrd++;

	/* Embed the error for recognizing a char. */
	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		if ( st->lmItemSet.length() == 1 && st->lmItemSet[0] != 0 ) {
			if ( st->isFinState() ) {
				/* On error execute the onActNext action, which knows that
				 * the last character of the token was one back and restart. */
				graph->setErrorTarget( st, graph->startState, &lmErrActionOrd, 
						&st->lmItemSet[0]->actOnNext, 1 );
				st->eofActionTable.setAction( lmErrActionOrd, 
						st->lmItemSet[0]->actOnNext );
				st->eofTarget = graph->startState;
			}
			else {
				graph->setErrorTarget( st, graph->startState, &lmErrActionOrd, 
						&st->lmItemSet[0]->actLagBehind, 1 );
				st->eofActionTable.setAction( lmErrActionOrd, 
						st->lmItemSet[0]->actLagBehind );
				st->eofTarget = graph->startState;
			}
		}
		else if ( st->lmItemSet.length() > 1 ) {
			/* Need to use the select. Take note of which items the select
			 * is needed for so only the necessary actions are included. */
			for ( LmItemSet::Iter plmi = st->lmItemSet; plmi.lte(); plmi++ ) {
				if ( *plmi != 0 )
					(*plmi)->inLmSelect = true;
			}
			/* On error, execute the action select and go to the start state. */
			graph->setErrorTarget( st, graph->startState, &lmErrActionOrd, 
					&lmActSelect, 1 );
			st->eofActionTable.setAction( lmErrActionOrd, lmActSelect );
			st->eofTarget = graph->startState;
		}
	}
	
	/* Finally, the start state should be made final. */
	graph->setFinState( graph->startState );
}

/* Build the individual machines, setting up the NFA transitions to final
 * states as we go. This is the base, unoptimized configuration. Later on we
 * look to eliminate NFA transitions. Return the union of all machines. */
FsmRes LongestMatch::buildBaseNfa( ParseData *pd )
{
	int nfaOrder = 1;
	FsmAp **parts = new FsmAp*[longestMatchList->length()];

	/* Make each part of the longest match. */
	LmPartList::Iter lmi = longestMatchList->last(); 
	for ( int i = longestMatchList->length() - 1; lmi.gtb(); lmi--, i-- ) {
		/* Create the machine and embed the setting of the longest match id. */
		FsmRes res = lmi->join->walk( pd );
		if ( !res.success() )
			return res;

		parts[i] = res.fsm;

		StateSet origFin = parts[i]->finStateSet;
		for ( StateSet::Iter fin = origFin; fin.lte(); fin++ ) {
			StateAp *orig = *fin;
			StateAp *newFinal = parts[i]->addState(); 

			newFinal->lmNfaParts.insert( lmi );
			
			NfaTrans *trans = new NfaTrans( nfaOrder++ );
			if ( orig->nfaOut == 0 )
				orig->nfaOut = new NfaTransList;
			orig->nfaOut->append( trans );
			parts[i]->attachToNfa( orig, newFinal, trans );

			if ( orig->outPriorTable.length() > 0 ) {
				newFinal->outPriorTable.insert( orig->outPriorTable );
				orig->outPriorTable.empty();
			}
			if ( orig->outActionTable.length() > 0 ) {
				newFinal->outActionTable.insert( orig->outActionTable );
				orig->outActionTable.empty();
			}
			if ( orig->outCondSpace != 0 ) {
				newFinal->outCondSpace = orig->outCondSpace;
				newFinal->outCondKeys.insert( orig->outCondKeys );
				orig->outCondSpace = 0;
				orig->outCondKeys.empty();
			}

			parts[i]->unsetFinState( orig );
			parts[i]->setFinState( newFinal );
		}
	}

	/* Union machines one and up with machine zero. The grammar dictates that
	 * there will always be at least one part. */
	FsmRes fsm( FsmRes::Fsm(), parts[0] );
	for ( int i = 1; i < longestMatchList->length(); i++ ) {
		fsm = FsmAp::unionOp( fsm, parts[i] );
		if ( !fsm.success() )
			return fsm;
	}

	/* Create a new, isolated start state into which we can embed tokstart
	 * functions. */
	fsm = FsmAp::isolateStartState( fsm );
	if ( !fsm.success() )
		return fsm;

	fsm->startState->toStateActionTable.setAction( pd->initTokStartOrd, pd->initTokStart );
	fsm->startState->fromStateActionTable.setAction( pd->setTokStartOrd, pd->setTokStart );

	KeyOps *keyOps = pd->fsmCtx->keyOps;

	/* Draw the trasition back to the start state. */
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		if ( st->lmNfaParts.length() > 0 ) {
			assert( st->lmNfaParts.length() == 1 );

			/*TransAp *newTrans = */fsm->attachNewTrans( st,
					fsm->startState, keyOps->minKey, keyOps->maxKey );

			fsm->transferOutData( st, st );
			if ( st->outCondSpace != 0 )
				FsmAp::embedCondition( fsm, st, st->outCondSpace->condSet, st->outCondKeys );

			for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
				if ( trans->plain() )
					trans->tdap()->actionTable.setAction( pd->fsmCtx->curActionOrd++, st->lmNfaParts[0]->actNfaOnNext );
				else {
					for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ )
						cond->actionTable.setAction( pd->fsmCtx->curActionOrd++, st->lmNfaParts[0]->actNfaOnNext );
				}
			}

			st->eofActionTable.setAction( pd->fsmCtx->curActionOrd++, st->lmNfaParts[0]->actNfaOnEof );
		}
	}

	delete[] parts;
	return fsm;
}

bool LongestMatch::matchCanFail( ParseData *pd, FsmAp *fsm, StateAp *st )
{
	if ( st->outCondSpace != 0 )
		return true;

	return false;
}


void LongestMatch::eliminateNfaActions( ParseData *pd, FsmAp *fsm )
{
	/*
	 * Once the union is complete we can optimize by advancing actions so they
	 * happen sooner, then draw the final transitions back to the start state.
	 * First step is to remove epsilon transitions that will never be taken. 
	 */
	bool modified = true;
	while ( modified ) {
		modified = false;

		for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
			/* Check if the nfa parts list is non-empty (meaning we have a final
			 * state created for matching a pattern). */
			if ( st->lmNfaParts.length() > 0 && st->nfaIn != 0 ) {
				/* Check if it can fail. If it can fail, then we cannot
				 * eliminate the prior candidates. If it can't fail then it is
				 * acceptable to eliminate the prior NFA transitions because we
				 * will never backtrack to follow them.*/ 
				if ( matchCanFail( pd, fsm, st ) )
					continue;

				for ( NfaInList::Iter in = *st->nfaIn; in.lte(); in++ ) {
					StateAp *fromState = in->fromState;
					/* Go forward until we get to the in-transition that cannot
					 * fail. Stop there because we are interested in what's
					 * before. */
					for ( NfaTransList::Iter to = *fromState->nfaOut; to.lte(); to++ ) {
						if ( to->order < in->order ) {
							/* Can nuke the epsilon transition that we will never
							 * follow. */
							fsm->detachFromNfa( fromState, to->toState, to );
							fromState->nfaOut->detach( to );
							delete to;

							modified = true;
							goto restart;
						}
					}
				}
			}
		}

		restart: {}
	}
}

bool LongestMatch::onlyOneNfa( ParseData *pd, FsmAp *fsm, StateAp *st, NfaTrans *in )
{
	if ( st->nfaOut != 0 && st->nfaOut->length() == 1 && st->nfaOut->head == in )
		return true;
	return false;
}

/* Advance NFA actions to the final character of the pattern match. This only
 * works when the machine cannot move forward more. */
void LongestMatch::advanceNfaActions( ParseData *pd, FsmAp *fsm )
{
	/*
	 * Advance actions to the final transition of the pattern match.
	 */
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		/* IS OUT COND SPACE ALL? */
		if ( st->lmNfaParts.length() > 0 && st->nfaIn != 0 ) {
			/* Only concern ourselves with final states that cannot fail. */
			if ( matchCanFail( pd, fsm, st ) )
				continue;

			/* If there are any out actions we cannot advance because we need
			 * to execute on the following character. We canot move to on-last,
			 * but in the next pass maybe we can eliminate the NFA action and
			 * move on leaving. */
			if ( st->outActionTable.length() > 0 )
				continue;

			for ( NfaInList::Iter in = *st->nfaIn; in.lte(); in++ ) {

				StateAp *fromState = in->fromState;
				if ( !fsm->anyRegularTransitions( fromState ) &&
						onlyOneNfa( pd, fsm, fromState, in ) )
				{
					/* Can nuke. */
					for ( TransInList::Iter t = fromState->inTrans; t.lte(); t++ ) {
						t->actionTable.setAction( pd->fsmCtx->curActionOrd++,
								st->lmNfaParts[0]->actNfaOnLast );
					}
					for ( CondInList::Iter t = fromState->inCond; t.lte(); t++ ) {
						t->actionTable.setAction( pd->fsmCtx->curActionOrd++,
								st->lmNfaParts[0]->actNfaOnLast );
					}

					fsm->moveInwardTrans( fsm->startState, fromState );
				}
			}
		}
	}
}


FsmRes LongestMatch::mergeNfaStates( ParseData *pd, FsmAp *fsm )
{
again:
	/*
	 * Advance actions to the final transition of the pattern match.
	 */
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		/* IS OUT COND SPACE ALL? */
		if ( st->lmNfaParts.length() > 0 && st->nfaIn != 0 ) {
			/* Only concern ourselves with final states that cannot fail. */
			if ( matchCanFail( pd, fsm, st ) )
				continue;

			for ( NfaInList::Iter in = *st->nfaIn; in.lte(); in++ ) {

				StateAp *fromState = in->fromState;
				if ( !fsm->anyRegularTransitions( fromState ) &&
						onlyOneNfa( pd, fsm, fromState, in ) )
				{
					/* Can apply the NFA transition, eliminating it. */
					FsmAp::applyNfaTrans( fsm, fromState, st, fromState->nfaOut->head );
					goto again;
				}
			}
		}
	}

	return FsmRes( FsmRes::Fsm(), fsm );
}

FsmRes LongestMatch::walkNfa( ParseData *pd )
{
	/* The longest match has it's own name scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Build the machines. */
	FsmRes fsm = buildBaseNfa( pd );
	if ( !fsm.success() )
		return fsm;

	/* Optimization passes. */
	eliminateNfaActions( pd, fsm );
	advanceNfaActions( pd, fsm );
	fsm = mergeNfaStates( pd, fsm );

	/* Pop the name scope. */
	pd->popNameScope( nameFrame );

	return fsm;
}
