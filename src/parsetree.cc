/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@complang.org>
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

using namespace std;
ostream &operator<<( ostream &out, const NameRef &nameRef );
ostream &operator<<( ostream &out, const NameInst &nameInst );

/* Read string literal (and regex) options and return the true end. */
const char *checkLitOptions( const InputLoc &loc, const char *data, int length, bool &caseInsensitive )
{
	const char *end = data + length - 1;
	while ( *end != '\'' && *end != '\"' && *end != '/' ) {
		if ( *end == 'i' )
			caseInsensitive = true;
		else {
			error( loc ) << "literal string '" << *end << 
					"' option not supported" << endl;
		}
		end -= 1;
	}
	return end;
}

/* Convert the literal string which comes in from the scanner into an array of
 * characters with escapes and options interpreted. Also null terminates the
 * string. Though this null termination should not be relied on for
 * interpreting literals in the parser because the string may contain \0 */
char *prepareLitString( const InputLoc &loc, const char *data, long length, 
		long &resLen, bool &caseInsensitive )
{
	char *resData = new char[length+1];
	caseInsensitive = false;

	const char *src = data + 1;
	const char *end = checkLitOptions( loc, data, length, caseInsensitive );

	char *dest = resData;
	long len = 0;
	while ( src != end ) {
		if ( *src == '\\' ) {
			switch ( src[1] ) {
			case '0': dest[len++] = '\0'; break;
			case 'a': dest[len++] = '\a'; break;
			case 'b': dest[len++] = '\b'; break;
			case 't': dest[len++] = '\t'; break;
			case 'n': dest[len++] = '\n'; break;
			case 'v': dest[len++] = '\v'; break;
			case 'f': dest[len++] = '\f'; break;
			case 'r': dest[len++] = '\r'; break;
			case '\n':  break;
			default: dest[len++] = src[1]; break;
			}
			src += 2;
		}
		else {
			dest[len++] = *src++;
		}
	}

	resLen = len;
	resData[resLen] = 0;
	return resData;
}

FsmRes VarDef::walk( ParseData *pd )
{
	/* We enter into a new name scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Recurse on the expression. */
	FsmRes rtnVal = machineDef->walk( pd );
	if ( !rtnVal.success() )
		return rtnVal;
	
	/* Do the tranfer of local error actions. */
	LocalErrDictEl *localErrDictEl = pd->localErrDict.find( name );
	if ( localErrDictEl != 0 ) {
		for ( StateList::Iter state = rtnVal.fsm->stateList; state.lte(); state++ )
			rtnVal.fsm->transferErrorActions( state, localErrDictEl->value );
	}

	/* If the expression below is a join operation with multiple expressions
	 * then it just had epsilon transisions resolved. If it is a join
	 * with only a single expression then run the epsilon op now. */
	if ( machineDef->type == MachineDef::JoinType &&
			machineDef->join->exprList.length() == 1 )
	{
		rtnVal = FsmAp::epsilonOp( rtnVal.fsm );
		if ( !rtnVal.success() )
			return rtnVal;
	}

	/* We can now unset entry points that are not longer used. */
	pd->unsetObsoleteEntries( rtnVal.fsm );

	/* If the name of the variable is referenced then add the entry point to
	 * the graph. */
	if ( pd->curNameInst->numRefs > 0 )
		rtnVal.fsm->setEntry( pd->curNameInst->id, rtnVal.fsm->startState );

	/* Pop the name scope. */
	pd->popNameScope( nameFrame );
	return rtnVal;
}

void VarDef::makeNameTree( const InputLoc &loc, ParseData *pd )
{
	/* The variable definition enters a new scope. */
	NameInst *prevNameInst = pd->curNameInst;
	pd->curNameInst = pd->addNameInst( loc, name, false );

	if ( machineDef->type == MachineDef::LongestMatchType )
		pd->curNameInst->isLongestMatch = true;

	/* Recurse. */
	machineDef->makeNameTree( pd );

	/* The name scope ends, pop the name instantiation. */
	pd->curNameInst = prevNameInst;
}

void VarDef::resolveNameRefs( ParseData *pd )
{
	/* Entering into a new scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Recurse. */
	machineDef->resolveNameRefs( pd );
	
	/* The name scope ends, pop the name instantiation. */
	pd->popNameScope( nameFrame );
}

VarDef::~VarDef()
{
	delete machineDef;
}

InputLoc LongestMatchPart::getLoc()
{ 
	return action != 0 ? action->loc : semiLoc;
}

/*
 * If there are any LMs then all of the following entry points must reset
 * tokstart:
 *
 *  1. fentry(StateRef)
 *  2. ftoto(StateRef), fcall(StateRef), fnext(StateRef)
 *  3. targt of any transition that has an fcall (the return loc).
 *  4. start state of all longest match routines.
 */

Action *LongestMatch::newAction( ParseData *pd, const InputLoc &loc, 
		const char *name, InlineList *inlineList )
{
	Action *action = new Action( loc, name, inlineList, pd->nextCondId++ );
	action->actionRefs.append( pd->curNameInst );
	pd->actionList.append( action );
	action->isLmAction = true;
	return action;
}

void LongestMatch::makeActions( ParseData *pd )
{
	/* Make actions that set the action id. */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		inlineList->head->children = new InlineList;
		inlineList->head->children->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmSetActId ) );
		char *actName = new char[50];
		sprintf( actName, "store%i", lmi->longestMatchId );
		lmi->setActId = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart on the last
	 * character. */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		inlineList->head->children = new InlineList;
		inlineList->head->children->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnLast ) );
		char *actName = new char[50];
		sprintf( actName, "last%i", lmi->longestMatchId );
		lmi->actOnLast = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart on the next
	 * character.  These actions will set tokend themselves (it is the current
	 * char). */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		inlineList->head->children = new InlineList;
		inlineList->head->children->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnNext ) );
		char *actName = new char[50];
		sprintf( actName, "next%i", lmi->longestMatchId );
		lmi->actOnNext = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart at tokend. These
	 * actions execute some time after matching the last char. */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		inlineList->head->children = new InlineList;
		inlineList->head->children->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnLagBehind ) );
		char *actName = new char[50];
		sprintf( actName, "lag%i", lmi->longestMatchId );
		lmi->actLagBehind = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	InputLoc loc;
	loc.line = 1;
	loc.col = 1;
	loc.fileName = "NONE";

	/* Create the error action. */
	InlineList *il6 = new InlineList;
	il6->append( new InlineItem( loc, this, 0, InlineItem::LmSwitch ) );
	lmActSelect = newAction( pd, loc, "switch", il6 );
}

void LongestMatch::findName( ParseData *pd )
{
	NameInst *nameInst = pd->curNameInst;
	while ( nameInst->name.empty() ) {
		nameInst = nameInst->parent;
		/* Since every machine must must have a name, we should always find a
		 * name for the longest match. */
		assert( nameInst != 0 );
	}
	name = nameInst->name;
}

void LongestMatch::makeNameTree( ParseData *pd )
{
	/* Create an anonymous scope for the longest match. Will be used for
	 * restarting machine after matching a token. */
	NameInst *prevNameInst = pd->curNameInst;
	pd->curNameInst = pd->addNameInst( loc, std::string(), false );

	/* Recurse into all parts of the longest match operator. */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ )
		lmi->join->makeNameTree( pd );

	/* Traverse the name tree upwards to find a name for this lm. */
	findName( pd );

	/* Also make the longest match's actions at this point. */
	makeActions( pd );

	/* The name scope ends, pop the name instantiation. */
	pd->curNameInst = prevNameInst;
}

void LongestMatch::resolveNameRefs( ParseData *pd )
{
	/* The longest match gets its own name scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Take an action reference for each longest match item and recurse. */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		/* Record the reference if the item has an action. */
		if ( lmi->action != 0 )
			lmi->action->actionRefs.append( pd->localNameScope );

		/* Recurse down the join. */
		lmi->join->resolveNameRefs( pd );
	}

	/* The name scope ends, pop the name instantiation. */
	pd->popNameScope( nameFrame );
}

void LongestMatch::restart( FsmAp *graph, TransAp *trans )
{
	if ( trans->plain() ) {
		StateAp *fromState = trans->tdap()->fromState;
		graph->detachTrans( fromState, trans->tdap()->toState, trans->tdap() );
		graph->attachTrans( fromState, graph->startState, trans->tdap() );
	}
	else {
		for ( CondList::Iter cti = trans->tcap()->condList; cti.lte(); cti++ ) {
			StateAp *fromState = cti->fromState;
			graph->detachTrans( fromState, cti->toState, cti );
			graph->attachTrans( fromState, graph->startState, cti );
		}
	}
}

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
				if ( trans->tdap()->lmActionTable.length() > 0 ) {
					LmActionTableEl *lmAct = trans->tdap()->lmActionTable.data;
					StateAp *toState = trans->tdap()->toState;
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
				/* FIXME: this needs a loop. */
				if ( trans->tcap()->condList.head->lmActionTable.length() > 0 ) {
					LmActionTableEl *lmAct = trans->tcap()->condList.head->lmActionTable.data;
					StateAp *toState = trans->tcap()->condList.head->toState;
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
		pd->lmRequiresErrorState = true;
		graph->startState->toStateActionTable.setAction( pd->initActIdOrd, pd->initActId );
	}

	/* The place to store transitions to restart. It maybe possible for the
	 * restarting to affect the searching through the graph that follows. For
	 * now take the safe route and save the list of transitions to restart
	 * until after all searching is done. */
	Vector<TransAp*> restartTrans;

	/* Set actions that do immediate token recognition, set the longest match part
	 * id and set the token ending. */
	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
			if ( trans->plain() ) {
				if ( trans->tdap()->lmActionTable.length() > 0 ) {
					LmActionTableEl *lmAct = trans->tdap()->lmActionTable.data;
					StateAp *toState = trans->tdap()->toState;
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
						trans->tdap()->actionTable.setAction( lmAct->key, 
								lmAct->value->actOnLast );
						restartTrans.append( trans );
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
							trans->tdap()->actionTable.setAction( pd->setTokEndOrd, pd->setTokEnd );

						/* Some states may not know which longest match item to
						 * execute, must set it. */
						if ( maxItemSetLength > 1 ) {
							/* There are transitions out, another match may come. */
							trans->tdap()->actionTable.setAction( lmAct->key, 
									lmAct->value->setActId );
						}
					}
				}
			}
			else {
				if ( trans->tcap()->condList.head->lmActionTable.length() > 0 ) {
					LmActionTableEl *lmAct = trans->tcap()->condList.head->lmActionTable.data;
					StateAp *toState = trans->tcap()->condList.head->toState;
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
						trans->tcap()->condList.head->actionTable.setAction( lmAct->key, 
								lmAct->value->actOnLast );
						restartTrans.append( trans );
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
							trans->tcap()->condList.head->actionTable.setAction( pd->setTokEndOrd, pd->setTokEnd );

						/* Some states may not know which longest match item to
						 * execute, must set it. */
						if ( maxItemSetLength > 1 ) {
							/* There are transitions out, another match may come. */
							trans->tcap()->condList.head->actionTable.setAction( lmAct->key, 
									lmAct->value->setActId );
						}
					}
				}
			}
		}
	}

	/* Now that all graph searching is done it certainly safe set the
	 * restarting. It may be safe above, however this must be verified. */
	for ( Vector<TransAp*>::Iter pt = restartTrans; pt.lte(); pt++ )
		restart( graph, *pt );

	int lmErrActionOrd = pd->curActionOrd++;

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

void LongestMatch::transferScannerLeavingActions( FsmAp *graph )
{
	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		if ( st->outActionTable.length() > 0 )
			graph->setErrorActions( st, st->outActionTable );
	}
}

FsmRes LongestMatch::walk( ParseData *pd )
{
	/* The longest match has it's own name scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Make each part of the longest match. */
	FsmAp **parts = new FsmAp*[longestMatchList->length()];
	LmPartList::Iter lmi = *longestMatchList; 
	for ( int i = 0; lmi.lte(); lmi++, i++ ) {
		/* Create the machine and embed the setting of the longest match id. */
		FsmRes res = lmi->join->walk( pd );
		if ( !res.success() )
			return res;

		parts[i] = res.fsm;
		parts[i]->longMatchAction( pd->curActionOrd++, lmi );
	}

	/* Before we union the patterns we need to deal with leaving actions. They
	 * are transfered to error transitions out of the final states (like local
	 * error actions) and to eof actions. In the scanner we need to forbid
	 * on_last for any final state that has an leaving action. */
	for ( int i = 0; i < longestMatchList->length(); i++ )
		transferScannerLeavingActions( parts[i] );

	/* Union machines one and up with machine zero. The grammar dictates that
	 * there will always be at least one part. */
	FsmRes res( FsmRes::Fsm(), parts[0] );
	for ( int i = 1; i < longestMatchList->length(); i++ ) {
		res = FsmAp::unionOp( res.fsm, parts[i] );
		if ( !res.success() )
			return res;

		afterOpMinimize( res.fsm );
	}

	runLongestMatch( pd, res.fsm );

	/* Pop the name scope. */
	pd->popNameScope( nameFrame );

	delete[] parts;
	return res;
}

NfaUnion::~NfaUnion()
{
	for ( TermVect::Iter term = terms; term.lte(); term++ )
		delete *term;
	if ( roundsList != 0 )
		delete roundsList;
}


FsmRes NfaUnion::condsDensity( ParseData *pd, FsmAp *fsm, StateAp *state, long depth )
{
	/* Nothing to do if the state is already on the list. */
	if ( state->stateBits & STB_ONLIST )
		return FsmRes( FsmRes::Fsm(), fsm );

	if ( depth > pd->id->nfaCondsDepth )
		return FsmRes( FsmRes::Fsm(), fsm );

	/* Doing depth first, put state on the list. */
	state->stateBits |= STB_ONLIST;

	/* Recurse on everything ranges. */
	for ( TransList::Iter trans = state->outList; trans.lte(); trans++ ) {
		if ( trans->plain() ) {
			if ( trans->tdap()->toState != 0 ) {
				FsmRes res = condsDensity( pd, fsm, trans->tdap()->toState, depth + 1 );
				if ( !res.success() )
					return res;
			}
		}
		else {
			for ( CondSet::Iter csi = trans->condSpace->condSet; csi.lte(); csi++ ) {
				if ( (*csi)->costMark )
					return FsmRes( FsmRes::CondCostTooHigh(), (*csi)->costId );
			}
			
			for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) {
				if ( cond->toState != 0 ) {
					FsmRes res = condsDensity( pd, fsm, cond->toState, depth + 1 );
					if ( !res.success() )
						return res;
				}
			}
		}
	}

	if ( state->nfaOut != 0 ) {
		for ( NfaTransList::Iter n = *state->nfaOut; n.lte(); n++ ) {
			/* We do not increment depth here since this is an epsilon transition. */
			FsmRes res = condsDensity( pd, fsm, n->toState, depth );
			if ( !res.success() )
				return res;
		}
	}

	for ( ActionTable::Iter a = state->fromStateActionTable; a.lte(); a++ ) {
		if ( a->value->costMark )
			return FsmRes( FsmRes::CondCostTooHigh(), a->value->costId );
	}

	return FsmRes( FsmRes::Fsm(), fsm );
}

void NfaUnion::transSpan( ParseData *pd, StateAp *state, long long &density, long depth )
{
	/* Nothing to do if the state is already on the list. */
	if ( state->stateBits & STB_ONLIST )
		return;

	if ( depth > pd->id->transSpanDepth )
		return;

	/* Recurse on everything ranges. */
	for ( TransList::Iter trans = state->outList; trans.lte(); trans++ ) {
		if ( trans->plain() ) {
			if ( trans->tdap()->toState != 0 ) {
				density += pd->fsmCtx->keyOps->span( trans->lowKey, trans->highKey );
				transSpan( pd, trans->tdap()->toState, density, depth + 1 );
			}
		}
		else {
			for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) {
				if ( cond->toState != 0 )
					transSpan( pd, cond->toState, density, depth + 1 );
			}
		}
	}

	if ( state->nfaOut != 0 ) {
		for ( NfaTransList::Iter n = *state->nfaOut; n.lte(); n++ ) {
			/* We do not increment depth here since this is an epsilon transition. */
			transSpan( pd, n->toState, density, depth );
		}
	}
}

FsmRes NfaUnion::condsDensity( ParseData *pd, FsmAp *fsmAp )
{
	/* Init on state list flags. */
	for ( StateList::Iter st = fsmAp->stateList; st.lte(); st++ )
		st->stateBits &= ~STB_ONLIST;

	return condsDensity( pd, fsmAp, fsmAp->startState, 1 );
}

FsmRes NfaUnion::walk( ParseData *pd )
{
	if ( pd->id->nfaTermCheck ) {
		/* Does not return. */
		nfaTermCheck( pd );
		return FsmRes( FsmRes::Aborted() );
	}

	if ( pd->id->nfaCondsDepth >= 0 ) {
		/* Does not return. */
		nfaCondsCheck( pd );
		return FsmRes( FsmRes::Aborted() );
	}

	if ( pd->id->nfaBreadthCheck ) {
		/* Does not return. */
		nfaBreadthCheck( pd );
		return FsmRes( FsmRes::Aborted() );
	}

	if ( pd->id->printStatistics )
		cout << "terms\t" << terms.length() << endl;

	/* Compute the individual expressions. */

	long numTerms = 0, sumPlain = 0, sumMin = 0;
	FsmAp **machines = new FsmAp*[terms.length()];
	for ( TermVect::Iter term = terms; term.lte(); term++ ) {
		FsmRes res = (*term)->walk( pd );
		machines[numTerms] = res.fsm;

		sumPlain += machines[numTerms]->stateList.length();

		machines[numTerms]->removeUnreachableStates();
		machines[numTerms]->minimizePartition2();
		sumMin += machines[numTerms]->stateList.length();

		//std::cout << (float)o * 100.0 / (float)terms.length() <<
		//	": " << machines[o]->stateList.length() << std::endl;

		//log << "term-state-list\t " <<
		//		machines[o]->stateList.length() << std::endl;

		numTerms += 1;
	}

	if ( pd->id->printStatistics ) {
		cout << "sum-plain\t" << sumPlain << endl;
		cout << "sum-minimized\t" << sumMin << endl;
	}

	/* For each round. */
	for ( NfaRoundVect::Iter r = *roundsList; r.lte(); r++ ) {
		if ( pd->id->printStatistics ) {
			cout << "depth\t" << r->depth << endl;
			cout << "grouping\t" << r->groups << endl;
		}

		int numGroups = 0;
		int start = 0;
		while ( start < numTerms ) {
			/* If nfa-group-max is zero, don't group, put all terms into a single
			 * n-depth NFA. */
			int amount = r->groups == 0 ? numTerms : r->groups;
			if ( ( start + amount ) > numTerms )
				amount = numTerms - start;

			FsmAp **others = machines + start + 1;
			FsmRes res = FsmAp::nfaUnionOp( machines[start], others, (amount - 1), r->depth );
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
		while ( start < numTerms ) {
			groups[g] = machines[start];
			start += r->groups == 0 ? numTerms : r->groups;
			g++;
		}

		delete[] machines;
		machines = groups;
		numTerms = numGroups;
	}

	FsmAp *ret = machines[0];
	return FsmRes( FsmRes::Fsm(), ret );
}

void nfaResultWrite( ostream &out, long code, long id, const char *scode )
{
	out << code << " " << id << " " << scode << endl;
}

void nfaCheckResult( ParseData *pd, long code, long id, const char *scode )
{
	stringstream out;
	nfaResultWrite( out, code, id, scode );
	pd->id->comm = out.str();
}

/* This is the first pass check. It looks for state (limit times 2 ) or
 * condition cost. We use this to expand generalized repetition to past the nfa
  union choice point. */
void NfaUnion::nfaCondsCheck( ParseData *pd )
{
	for ( TermVect::Iter term = terms; term.lte(); term++ ) {
		pd->fsmCtx->stateLimit = pd->id->nfaIntermedStateLimit * 2;
		FsmRes res = (*term)->walk( pd );
		pd->fsmCtx->stateLimit = -1;

		if ( !res.success() ) {
			if ( res.type == FsmRes::TypeTooManyStates )
				nfaCheckResult( pd, 1, 0, "too-many-states" );
			else if ( res.type == FsmRes::TypeRepetitionError )
				nfaCheckResult( pd, 2, 0, "rep-error" );
			return;
		}

		FsmRes condsRes = condsDensity( pd, res.fsm );
		delete res.fsm;

		if ( !condsRes.success() ) {
			if ( condsRes.type == FsmRes::TypeCondCostTooHigh )
				nfaCheckResult( pd, 20, condsRes.id, "cond-cost" );
			else if ( condsRes.type == FsmRes::TypeRepetitionError )
				nfaCheckResult( pd, 2, 0, "rep-error" );
			return;
		}
	}

	nfaCheckResult( pd, 0, 0, "OK" );
}


void NfaUnion::nfaTermCheck( ParseData *pd )
{
	for ( TermVect::Iter term = terms; term.lte(); term++ ) {
		pd->fsmCtx->stateLimit = pd->id->nfaIntermedStateLimit;
		FsmRes res = (*term)->walk( pd );
		pd->fsmCtx->stateLimit = -1;

		if ( !res.success() ) {
			if ( res.type == FsmRes::TypeTooManyStates )
				nfaCheckResult( pd, 1, 0, "too-many-states" );
			else if ( res.type == FsmRes::TypePriorInteraction )
				nfaCheckResult( pd, 60, res.id, "prior-interaction" );
			else if ( res.type == FsmRes::TypeRepetitionError )
				nfaCheckResult( pd, 2, 0, "rep-error" );
			return;
		}
		delete res.fsm;
	}

	nfaCheckResult( pd, 0, 0, "OK" );
}

/*
 * This algorithm assigns a price to each state visit, then adds that to a
 * running total. Note that we do not guard against multiple visits to a state,
 * since we are estimating runtime cost.
 *
 * We rely on a character histogram and are looking for a probability of being
 * in any given state, given that histogram, simple and very effective.
 */
void NfaUnion::checkBreadth( ParseData *pd, FsmAp *fsm, StateAp *state,
		long depth, int maxDepth, double stateScore, double &total )
{
	if ( depth > maxDepth )
		return;
	
	/* Recurse on everything ranges. */
	for ( TransList::Iter trans = state->outList; trans.lte(); trans++ ) {
		/* Compute target state score. */
		double span = 0;
		for ( int i = trans->lowKey.getVal(); i <= trans->highKey.getVal(); i++ )
			span += pd->id->histogram[i];

		double targetStateScore = stateScore * ( span );

		/* Add to the level. */
		total += targetStateScore;

		if ( trans->plain() ) {
			if ( trans->tdap()->toState != 0 ) {
				checkBreadth( pd, fsm, trans->tdap()->toState,
						depth + 1, maxDepth, targetStateScore, total );
			}
		}
		else {
			for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) {
				if ( cond->toState != 0 ) {
					checkBreadth( pd, fsm, cond->toState,
							depth + 1, maxDepth, targetStateScore, total );
				}
			}
		}
	}

	if ( state->nfaOut != 0 ) {
		for ( NfaTransList::Iter n = *state->nfaOut; n.lte(); n++ ) {
			/* We do not increment depth here since this is an epsilon transition. */
			checkBreadth( pd, fsm, n->toState, depth, maxDepth, stateScore, total );
		}
	}
}

double NfaUnion::checkBreadth( ParseData *pd, FsmAp *fsm, StateAp *state )
{
	const int maxDepth = 5;
	double total = 0;
	checkBreadth( pd, fsm, state, 1, maxDepth, 1.0, total );
	return total;
}

void NfaUnion::checkBreadth( ParseData *pd, FsmAp *fsm )
{
	int exitCode = 21;
	double total = checkBreadth( pd, fsm, fsm->startState );

	/* This will exit. Can't call it because we need to perform the score
	 * checks after. */
	nfaCheckResult( pd, exitCode, 1, "OK" );

	ostream *out = &cout;
	ofstream *ofs = 0;
	if ( pd->id->commFileName != 0 )
		out = ofs = new ofstream( pd->id->commFileName, std::fstream::app );

	*out << std::fixed << std::setprecision(0);
	double start = total;
	
	for ( Vector<ParseData::Cut>::Iter c = pd->cuts; c.lte(); c++ ) {
		for ( EntryMap::Iter mel = fsm->entryPoints; mel.lte(); mel++ ) {
			if ( mel->key == c->entryId ) {
				total = checkBreadth( pd, fsm, mel->value );

				if ( start > 0.01 ) {
					*out << "COST " << c->name << " " <<
							( 1000000.0 * start ) << " " << 
							( 1000000.0 * ( total / start ) ) << endl;
				}
			}
		}
	}

	if ( ofs != 0 ) {
		ofs->close();
		delete ofs;
	}
}

void NfaUnion::nfaBreadthCheck( ParseData *pd )
{
	for ( TermVect::Iter term = terms; term.lte(); term++ ) {
		FsmAp *fsm = 0;

		/* No need for state limit here since this check is used after all
		 * others pass. One check only and . */
		FsmRes res = (*term)->walk( pd );
		fsm = res.fsm;
		checkBreadth( pd, fsm );
		delete fsm;
		return;
	}

	nfaCheckResult( pd, 0, 0, "OK" );
}

void NfaUnion::makeNameTree( ParseData *pd )
{
	for ( TermVect::Iter term = terms; term.lte(); term++ )
		(*term)->makeNameTree( pd );
}

void NfaUnion::resolveNameRefs( ParseData *pd )
{
	for ( TermVect::Iter term = terms; term.lte(); term++ )
		(*term)->resolveNameRefs( pd );
}

FsmRes MachineDef::walk( ParseData *pd )
{
	switch ( type ) {
	case JoinType:
		return join->walk( pd );
	case LongestMatchType:
		return longestMatch->walk( pd );
	case LengthDefType:
		/* Towards lengths. */
		return FsmRes( FsmRes::Fsm(), FsmAp::lambdaFsm( pd->fsmCtx ) );
	case NfaUnionType:
		return nfaUnion->walk( pd );
	}
	return FsmRes( FsmRes::Aborted() );
}

void MachineDef::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case JoinType:
		join->makeNameTree( pd );
		break;
	case LongestMatchType:
		longestMatch->makeNameTree( pd );
		break;
	case LengthDefType:
		break;
	case NfaUnionType:
		nfaUnion->makeNameTree( pd );
		break;
	}
}

void MachineDef::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case JoinType:
		join->resolveNameRefs( pd );
		break;
	case LongestMatchType:
		longestMatch->resolveNameRefs( pd );
		break;
	case LengthDefType:
		break;
	case NfaUnionType:
		nfaUnion->resolveNameRefs( pd );
		break;
	}
}

MachineDef::~MachineDef()
{
	if ( join != 0 )
		delete join;
	if ( longestMatch != 0 )
		delete longestMatch;
	if ( lengthDef != 0 )
		delete lengthDef;
	if ( nfaUnion != 0 )
		delete nfaUnion;
}

/* Construct with a location and the first expression. */
Join::Join( const InputLoc &loc, Expression *expr )
:
	loc(loc)
{
	exprList.append( expr );
}

/* Construct with a location and the first expression. */
Join::Join( Expression *expr )
{
	exprList.append( expr );
}

/* Walk an expression node. */
FsmRes Join::walk( ParseData *pd )
{
	if ( exprList.length() == 1 )
		return exprList.head->walk( pd );

	return walkJoin( pd );
}

/* There is a list of expressions to join. */
FsmRes Join::walkJoin( ParseData *pd )
{
	/* We enter into a new name scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Evaluate the machines. */
	FsmAp **fsms = new FsmAp*[exprList.length()];
	ExprList::Iter expr = exprList;
	for ( int e = 0; e < exprList.length(); e++, expr++ ) {
		FsmRes res = expr->walk( pd );
		if ( !res.success() )
			return res;
		fsms[e] = res.fsm;
	}
	
	/* Get the start and final names. Final is 
	 * guaranteed to exist, start is not. */
	NameInst *startName = pd->curNameInst->start;
	NameInst *finalName = pd->curNameInst->final;

	int startId = -1;
	if ( startName != 0 ) {
		/* Take note that there was an implicit link to the start machine. */
		pd->localNameScope->referencedNames.append( startName );
		startId = startName->id;
	}

	/* A final id of -1 indicates there is no epsilon that references the
	 * final state, therefor do not create one or set an entry point to it. */
	int finalId = -1;
	if ( finalName->numRefs > 0 )
		finalId = finalName->id;

	/* Join machines 1 and up onto machine 0. */
	FsmRes res = FsmAp::joinOp( fsms[0], startId, finalId, fsms+1, exprList.length()-1 );
	if ( !res.success() )
		return res;

	/* We can now unset entry points that are not longer used. */
	pd->unsetObsoleteEntries( res.fsm );

	/* Pop the name scope. */
	pd->popNameScope( nameFrame );

	delete[] fsms;
	return res;
}

void Join::makeNameTree( ParseData *pd )
{
	if ( exprList.length() > 1 ) {
		/* Create the new anonymous scope. */
		NameInst *prevNameInst = pd->curNameInst;
		pd->curNameInst = pd->addNameInst( loc, std::string(), false );

		/* Join scopes need an implicit "final" target. */
		pd->curNameInst->final = new NameInst( InputLoc(), pd->curNameInst, "final", 
				pd->nextNameId++, false );

		/* Recurse into all expressions in the list. */
		for ( ExprList::Iter expr = exprList; expr.lte(); expr++ )
			expr->makeNameTree( pd );

		/* The name scope ends, pop the name instantiation. */
		pd->curNameInst = prevNameInst;
	}
	else {
		/* Recurse into the single expression. */
		exprList.head->makeNameTree( pd );
	}
}


void Join::resolveNameRefs( ParseData *pd )
{
	/* Branch on whether or not there is to be a join. */
	if ( exprList.length() > 1 ) {
		/* The variable definition enters a new scope. */
		NameFrame nameFrame = pd->enterNameScope( true, 1 );

		/* The join scope must contain a start label. */
		NameSet resolved = pd->resolvePart( pd->localNameScope, "start", true );
		if ( resolved.length() > 0 ) {
			/* Take the first. */
			pd->curNameInst->start = resolved[0];
			if ( resolved.length() > 1 ) {
				/* Complain about the multiple references. */
				error(loc) << "join operation has multiple start labels" << endl;
				errorStateLabels( resolved );
			}
		}

		/* Make sure there is a start label. */
		if ( pd->curNameInst->start != 0 ) {
			/* There is an implicit reference to start name. */
			pd->curNameInst->start->numRefs += 1;
		}
		else {
			/* No start label. */
			error(loc) << "join operation has no start label" << endl;
		}

		/* Recurse into all expressions in the list. */
		for ( ExprList::Iter expr = exprList; expr.lte(); expr++ )
			expr->resolveNameRefs( pd );

		/* The name scope ends, pop the name instantiation. */
		pd->popNameScope( nameFrame );
	}
	else {
		/* Recurse into the single expression. */
		exprList.head->resolveNameRefs( pd );
	}
}

/* Clean up after an expression node. */
Expression::~Expression()
{
	if ( expression )
		delete expression;
	if ( term )
		delete term;
}

/* Evaluate a single expression node. */
FsmRes Expression::walk( ParseData *pd, bool lastInSeq )
{
	switch ( type ) {
		case OrType: {
			/* Evaluate the expression. */
			FsmRes exprFsm = expression->walk( pd, false );
			if ( !exprFsm.success() )
				return exprFsm;

			/* Evaluate the term. */
			FsmRes rhs = term->walk( pd );
			if ( !rhs.success() )
				return rhs;

			/* Perform union. */
			FsmRes res = FsmAp::unionOp( exprFsm.fsm, rhs.fsm );
			if ( !res.success() )
				return res;

			afterOpMinimize( res.fsm, lastInSeq );

			return res;
		}
		case IntersectType: {
			/* Evaluate the expression. */
			FsmRes exprFsm = expression->walk( pd );
			if ( !exprFsm.success() )
				return exprFsm;

			/* Evaluate the term. */
			FsmRes rhs = term->walk( pd );
			if ( !rhs.success() )
				return rhs;

			/* Perform intersection. */
			FsmRes res = FsmAp::intersectOp( exprFsm.fsm, rhs.fsm );
			if ( !res.success() )
				return res;

			afterOpMinimize( res.fsm, lastInSeq );
			return res;
		}
		case SubtractType: {
			/* Evaluate the expression. */
			FsmRes exprFsm = expression->walk( pd );
			if ( !exprFsm.success() )
				return exprFsm;

			/* Evaluate the term. */
			FsmRes rhs = term->walk( pd );
			if ( !rhs.success() )
				return rhs;

			/* Perform subtraction. */
			FsmRes res = FsmAp::subtractOp( exprFsm.fsm, rhs.fsm );
			if ( !res.success() )
				return res;

			afterOpMinimize( res.fsm, lastInSeq );
			return res;
		}
		case StrongSubtractType: {
			/* Evaluate the expression. */
			FsmRes exprFsm = expression->walk( pd );
			if ( !exprFsm.success() )
				return exprFsm;

			FsmAp *leadAnyStar = dotStarFsm( pd );
			FsmAp *trailAnyStar = dotStarFsm( pd );

			/* Evaluate the term and pad it with any* machines. */
			FsmRes termFsm = term->walk( pd );
			if ( !termFsm.success() )
				return termFsm;

			FsmRes res1 = FsmAp::concatOp( leadAnyStar, termFsm.fsm );
			if ( !res1.success() )
				return res1;

			FsmRes res2 = FsmAp::concatOp( res1.fsm, trailAnyStar );
			if ( !res2.success() )
				return res2;

			/* Perform subtraction. */
			FsmRes res3 = FsmAp::subtractOp( exprFsm.fsm, res2.fsm );
			if ( !res3.success() )
				return res3;

			afterOpMinimize( res3.fsm, lastInSeq );
			return res3;
		}
		case TermType: {
			/* Return result of the term. */
			return term->walk( pd );
		}
		case BuiltinType: {
			/* Construct the builtin. */
			return FsmRes( FsmRes::Fsm(), makeBuiltin( builtin, pd ) );
		}
	}

	return FsmRes( FsmRes::Aborted() );
}

void Expression::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case OrType:
	case IntersectType:
	case SubtractType:
	case StrongSubtractType:
		expression->makeNameTree( pd );
		term->makeNameTree( pd );
		break;
	case TermType:
		term->makeNameTree( pd );
		break;
	case BuiltinType:
		break;
	}
}

void Expression::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case OrType:
	case IntersectType:
	case SubtractType:
	case StrongSubtractType:
		expression->resolveNameRefs( pd );
		term->resolveNameRefs( pd );
		break;
	case TermType:
		term->resolveNameRefs( pd );
		break;
	case BuiltinType:
		break;
	}
}

/* Clean up after a term node. */
Term::~Term()
{
	if ( term )
		delete term;
	if ( factorWithAug )
		delete factorWithAug;
}

/* Evaluate a term node. */
FsmRes Term::walk( ParseData *pd, bool lastInSeq )
{
	switch ( type ) {
		case ConcatType: {
			/* Evaluate the Term. */
			FsmRes termFsm = term->walk( pd, false );
			if ( !termFsm.success() )
				return termFsm;

			/* Evaluate the FactorWithRep. */
			FsmRes rhs = factorWithAug->walk( pd );
			if ( !rhs.success() ) {
				delete termFsm.fsm;
				return rhs;
			}

			/* Perform concatenation. */
			FsmRes res = FsmAp::concatOp( termFsm.fsm, rhs.fsm );
			if ( !res.success() )
				return res;

			afterOpMinimize( res.fsm, lastInSeq );
			return res;
		}
		case RightStartType: {
			/* Evaluate the Term. */
			FsmRes termFsm = term->walk( pd );
			if ( !termFsm.success() )
				return termFsm;

			/* Evaluate the FactorWithRep. */
			FsmRes rhs = factorWithAug->walk( pd );
			if ( !rhs.success() ) {
				delete termFsm.fsm;
				return rhs;
			}

			/* Set up the priority descriptors. The left machine gets the
			 * lower priority where as the right get the higher start priority. */
			priorDescs[0].key = pd->nextPriorKey++;
			priorDescs[0].priority = 0;
			termFsm.fsm->allTransPrior( pd->curPriorOrd++, &priorDescs[0] );

			/* The start transitions of the right machine gets the higher
			 * priority. Use the same unique key. */
			priorDescs[1].key = priorDescs[0].key;
			priorDescs[1].priority = 1;
			rhs.fsm->startFsmPrior( pd->curPriorOrd++, &priorDescs[1] );

			/* Perform concatenation. */
			FsmRes res = FsmAp::concatOp( termFsm.fsm, rhs.fsm );
			if ( !res.success() )
				return res;

			afterOpMinimize( res.fsm, lastInSeq );
			return res;
		}
		case RightFinishType: {
			/* Evaluate the Term. */
			FsmRes termFsm = term->walk( pd );
			if ( !termFsm.success() )
				return termFsm;

			/* Evaluate the FactorWithRep. */
			FsmRes rhs = factorWithAug->walk( pd );
			if ( !rhs.success() ) {
				delete termFsm.fsm;
				return rhs;
			}

			/* Set up the priority descriptors. The left machine gets the
			 * lower priority where as the finishing transitions to the right
			 * get the higher priority. */
			priorDescs[0].key = pd->nextPriorKey++;
			priorDescs[0].priority = 0;
			termFsm.fsm->allTransPrior( pd->curPriorOrd++, &priorDescs[0] );

			/* The finishing transitions of the right machine get the higher
			 * priority. Use the same unique key. */
			priorDescs[1].key = priorDescs[0].key;
			priorDescs[1].priority = 1;
			rhs.fsm->finishFsmPrior( pd->curPriorOrd++, &priorDescs[1] );

			/* If the right machine's start state is final we need to guard
			 * against the left machine persisting by moving through the empty
			 * string. */
			if ( rhs.fsm->startState->isFinState() ) {
				rhs.fsm->startState->outPriorTable.setPrior( 
						pd->curPriorOrd++, &priorDescs[1] );
			}

			/* Perform concatenation. */
			FsmRes res = FsmAp::concatOp( termFsm.fsm, rhs.fsm );
			if ( !res.success() ) 
				return res;

			afterOpMinimize( res.fsm, lastInSeq );
			return res;
		}
		case LeftType: {
			/* Evaluate the Term. */
			FsmRes termFsm = term->walk( pd );
			if ( !termFsm.success() )
				return termFsm;

			/* Evaluate the FactorWithRep. */
			FsmRes rhs = factorWithAug->walk( pd );
			if ( !rhs.success() ) {
				delete termFsm.fsm;
				return rhs;
			}

			/* Set up the priority descriptors. The left machine gets the
			 * higher priority. */
			priorDescs[0].key = pd->nextPriorKey++;
			priorDescs[0].priority = 1;
			termFsm.fsm->allTransPrior( pd->curPriorOrd++, &priorDescs[0] );

			/* The right machine gets the lower priority. We cannot use
			 * allTransPrior here in case the start state of the right machine
			 * is final. It would allow the right machine thread to run along
			 * with the left if just passing through the start state. Using
			 * startFsmPrior prevents this. */
			priorDescs[1].key = priorDescs[0].key;
			priorDescs[1].priority = 0;
			rhs.fsm->startFsmPrior( pd->curPriorOrd++, &priorDescs[1] );

			/* Perform concatenation. */
			FsmRes res = FsmAp::concatOp( termFsm.fsm, rhs.fsm );
			if ( !res.success() )
				return res;

			afterOpMinimize( res.fsm, lastInSeq );
			return res;
		}
		case FactorWithAugType: {
			return factorWithAug->walk( pd );
		}
	}
	return FsmRes( FsmRes::Aborted() );
}

void Term::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case ConcatType:
	case RightStartType:
	case RightFinishType:
	case LeftType:
		term->makeNameTree( pd );
		factorWithAug->makeNameTree( pd );
		break;
	case FactorWithAugType:
		factorWithAug->makeNameTree( pd );
		break;
	}
}

void Term::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case ConcatType:
	case RightStartType:
	case RightFinishType:
	case LeftType:
		term->resolveNameRefs( pd );
		factorWithAug->resolveNameRefs( pd );
		break;
	case FactorWithAugType:
		factorWithAug->resolveNameRefs( pd );
		break;
	}
}

/* Clean up after a factor with augmentation node. */
FactorWithAug::~FactorWithAug()
{
	delete factorWithRep;

	/* Walk the vector of parser actions, deleting function names. */

	/* Clean up priority descriptors. */
	if ( priorDescs != 0 )
		delete[] priorDescs;
}

void FactorWithAug::assignActions( ParseData *pd, FsmAp *graph, int *actionOrd )
{
	/* Assign actions. */
	for ( int i = 0; i < actions.length(); i++ )  {
		switch ( actions[i].type ) {
		/* Transition actions. */
		case at_start:
			graph->startFsmAction( actionOrd[i], actions[i].action );
			afterOpMinimize( graph );
			break;
		case at_all:
			graph->allTransAction( actionOrd[i], actions[i].action );
			break;
		case at_finish:
			graph->finishFsmAction( actionOrd[i], actions[i].action );
			break;
		case at_leave:
			graph->leaveFsmAction( actionOrd[i], actions[i].action );
			break;

		/* Global error actions. */
		case at_start_gbl_error:
			graph->startErrorAction( actionOrd[i], actions[i].action, 0 );
			afterOpMinimize( graph );
			break;
		case at_all_gbl_error:
			graph->allErrorAction( actionOrd[i], actions[i].action, 0 );
			break;
		case at_final_gbl_error:
			graph->finalErrorAction( actionOrd[i], actions[i].action, 0 );
			break;
		case at_not_start_gbl_error:
			graph->notStartErrorAction( actionOrd[i], actions[i].action, 0 );
			break;
		case at_not_final_gbl_error:
			graph->notFinalErrorAction( actionOrd[i], actions[i].action, 0 );
			break;
		case at_middle_gbl_error:
			graph->middleErrorAction( actionOrd[i], actions[i].action, 0 );
			break;

		/* Local error actions. */
		case at_start_local_error:
			graph->startErrorAction( actionOrd[i], actions[i].action,
					actions[i].localErrKey );
			afterOpMinimize( graph );
			break;
		case at_all_local_error:
			graph->allErrorAction( actionOrd[i], actions[i].action,
					actions[i].localErrKey );
			break;
		case at_final_local_error:
			graph->finalErrorAction( actionOrd[i], actions[i].action,
					actions[i].localErrKey );
			break;
		case at_not_start_local_error:
			graph->notStartErrorAction( actionOrd[i], actions[i].action,
					actions[i].localErrKey );
			break;
		case at_not_final_local_error:
			graph->notFinalErrorAction( actionOrd[i], actions[i].action,
					actions[i].localErrKey );
			break;
		case at_middle_local_error:
			graph->middleErrorAction( actionOrd[i], actions[i].action,
					actions[i].localErrKey );
			break;

		/* EOF actions. */
		case at_start_eof:
			graph->startEOFAction( actionOrd[i], actions[i].action );
			afterOpMinimize( graph );
			break;
		case at_all_eof:
			graph->allEOFAction( actionOrd[i], actions[i].action );
			break;
		case at_final_eof:
			graph->finalEOFAction( actionOrd[i], actions[i].action );
			break;
		case at_not_start_eof:
			graph->notStartEOFAction( actionOrd[i], actions[i].action );
			break;
		case at_not_final_eof:
			graph->notFinalEOFAction( actionOrd[i], actions[i].action );
			break;
		case at_middle_eof:
			graph->middleEOFAction( actionOrd[i], actions[i].action );
			break;

		/* To State Actions. */
		case at_start_to_state:
			graph->startToStateAction( actionOrd[i], actions[i].action );
			afterOpMinimize( graph );
			break;
		case at_all_to_state:
			graph->allToStateAction( actionOrd[i], actions[i].action );
			break;
		case at_final_to_state:
			graph->finalToStateAction( actionOrd[i], actions[i].action );
			break;
		case at_not_start_to_state:
			graph->notStartToStateAction( actionOrd[i], actions[i].action );
			break;
		case at_not_final_to_state:
			graph->notFinalToStateAction( actionOrd[i], actions[i].action );
			break;
		case at_middle_to_state:
			graph->middleToStateAction( actionOrd[i], actions[i].action );
			break;

		/* From State Actions. */
		case at_start_from_state:
			graph->startFromStateAction( actionOrd[i], actions[i].action );
			afterOpMinimize( graph );
			break;
		case at_all_from_state:
			graph->allFromStateAction( actionOrd[i], actions[i].action );
			break;
		case at_final_from_state:
			graph->finalFromStateAction( actionOrd[i], actions[i].action );
			break;
		case at_not_start_from_state:
			graph->notStartFromStateAction( actionOrd[i], actions[i].action );
			break;
		case at_not_final_from_state:
			graph->notFinalFromStateAction( actionOrd[i], actions[i].action );
			break;
		case at_middle_from_state:
			graph->middleFromStateAction( actionOrd[i], actions[i].action );
			break;

		/* Remaining cases, prevented by the parser. */
		default: 
			assert( false );
			break;
		}
	}
}

void FactorWithAug::assignPriorities( FsmAp *graph, int *priorOrd )
{
	/* Assign priorities. */
	for ( int i = 0; i < priorityAugs.length(); i++ ) {
		switch ( priorityAugs[i].type ) {
		case at_start:
			graph->startFsmPrior( priorOrd[i], &priorDescs[i]);
			/* Start fsm priorities are a special case that may require
			 * minimization afterwards. */
			afterOpMinimize( graph );
			break;
		case at_all:
			graph->allTransPrior( priorOrd[i], &priorDescs[i] );
			break;
		case at_finish:
			graph->finishFsmPrior( priorOrd[i], &priorDescs[i] );
			break;
		case at_leave:
			graph->leaveFsmPrior( priorOrd[i], &priorDescs[i] );
			break;

		default:
			/* Parser Prevents this case. */
			break;
		}
	}
}

void FactorWithAug::assignConditions( FsmAp *graph )
{
	for ( int i = 0; i < conditions.length(); i++ )  {
		switch ( conditions[i].type ) {
		/* Transition actions. */
		case at_start:
			graph->startFsmCondition( conditions[i].action, conditions[i].sense );
			afterOpMinimize( graph );
			break;
		case at_all:
			graph->allTransCondition( conditions[i].action, conditions[i].sense );
			break;
		case at_leave:
			graph->leaveFsmCondition( conditions[i].action, conditions[i].sense );
			break;
		default:
			break;
		}
	}
}

/* Evaluate a factor with augmentation node. */
FsmRes FactorWithAug::walk( ParseData *pd )
{
	/* Enter into the scopes created for the labels. */
	NameFrame nameFrame = pd->enterNameScope( false, labels.length() );

	/* Make the array of function orderings. */
	int *actionOrd = 0;
	if ( actions.length() > 0 )
		actionOrd = new int[actions.length()];
	
	/* First walk the list of actions, assigning order to all starting
	 * actions. */
	for ( int i = 0; i < actions.length(); i++ ) {
		if ( actions[i].type == at_start || 
				actions[i].type == at_start_gbl_error ||
				actions[i].type == at_start_local_error ||
				actions[i].type == at_start_to_state ||
				actions[i].type == at_start_from_state ||
				actions[i].type == at_start_eof )
			actionOrd[i] = pd->curActionOrd++;
	}

	/* Evaluate the factor with repetition. */
	FsmRes factorTree = factorWithRep->walk( pd );
	if ( !factorTree.success() ) {
		delete [] actionOrd;
		return factorTree;
	}

	FsmAp *rtnVal = factorTree.fsm;

	/* Compute the remaining action orderings. */
	for ( int i = 0; i < actions.length(); i++ ) {
		if ( actions[i].type != at_start && 
				actions[i].type != at_start_gbl_error &&
				actions[i].type != at_start_local_error &&
				actions[i].type != at_start_to_state &&
				actions[i].type != at_start_from_state &&
				actions[i].type != at_start_eof )
			actionOrd[i] = pd->curActionOrd++;
	}

	/* Embed conditions. */
	assignConditions( rtnVal );

	/* Embed actions. */
	assignActions( pd, rtnVal , actionOrd );

	/* Make the array of priority orderings. Orderings are local to this walk
	 * of the factor with augmentation. */
	int *priorOrd = 0;
	if ( priorityAugs.length() > 0 )
		priorOrd = new int[priorityAugs.length()];
	
	/* Walk all priorities, assigning the priority ordering. */
	for ( int i = 0; i < priorityAugs.length(); i++ )
		priorOrd[i] = pd->curPriorOrd++;

	/* If the priority descriptors have not been made, make them now.  Make
	 * priority descriptors for each priority asignment that will be passed to
	 * the fsm. Used to keep track of the key, value and used bit. */
	if ( priorDescs == 0 && priorityAugs.length() > 0 ) {
		priorDescs = new PriorDesc[priorityAugs.length()];
		for ( int i = 0; i < priorityAugs.length(); i++ ) {
			/* Init the prior descriptor for the priority setting. */
			priorDescs[i].key = priorityAugs[i].priorKey;
			priorDescs[i].priority = priorityAugs[i].priorValue;
			priorDescs[i].guarded = false;
			priorDescs[i].guardId = 0;
		}
	}

	/* Assign priorities into the machine. */
	assignPriorities( rtnVal, priorOrd );

	/* Assign epsilon transitions. */
	for ( int e = 0; e < epsilonLinks.length(); e++ ) {
		/* Get the name, which may not exist. If it doesn't then silently
		 * ignore it because an error has already been reported. */
		NameInst *epTarg = pd->epsilonResolvedLinks[pd->nextEpsilonResolvedLink++];
		if ( epTarg != 0 ) {
			/* Make the epsilon transitions. */
			rtnVal->epsilonTrans( epTarg->id );

			/* Note that we have made a link to the name. */
			pd->localNameScope->referencedNames.append( epTarg );
		}
	}

	/* Set entry points for labels. */
	if ( labels.length() > 0 ) {
		/* Pop the names. */
		pd->resetNameScope( nameFrame );

		/* Make labels that are referenced into entry points. */
		for ( int i = 0; i < labels.length(); i++ ) {
			pd->enterNameScope( false, 1 );

			/* Will always be found. */
			NameInst *name = pd->curNameInst;

			/* If the name is referenced then set the entry point. */
			if ( name->numRefs > 0 )
				rtnVal->setEntry( name->id, rtnVal->startState );

			if ( labels[i].cut )
				pd->cuts.append( ParseData::Cut( labels[i].data, name->id ) );
		}

		pd->popNameScope( nameFrame );
	}

	if ( priorOrd != 0 )
		delete[] priorOrd;
	if ( actionOrd != 0 )
		delete[] actionOrd;	
	return FsmRes( FsmRes::Fsm(), rtnVal );
}

void FactorWithAug::makeNameTree( ParseData *pd )
{
	/* Add the labels to the tree of instantiated names. Each label
	 * makes a new scope. */
	NameInst *prevNameInst = pd->curNameInst;
	for ( int i = 0; i < labels.length(); i++ ) {
		pd->curNameInst = pd->addNameInst( labels[i].loc, labels[i].data, true );

		if ( labels[i].cut )
			pd->curNameInst->numRefs += 1;
	}

	/* Recurse, then pop the names. */
	factorWithRep->makeNameTree( pd );
	pd->curNameInst = prevNameInst;
}


void FactorWithAug::resolveNameRefs( ParseData *pd )
{
	/* Enter into the name scope created by any labels. */
	NameFrame nameFrame = pd->enterNameScope( false, labels.length() );

	/* Note action references. */
	for ( int i = 0; i < actions.length(); i++ ) 
		actions[i].action->actionRefs.append( pd->localNameScope );

	/* Recurse first. IMPORTANT: we must do the exact same traversal as when
	 * the tree is constructed. */
	factorWithRep->resolveNameRefs( pd );

	/* Resolve epsilon transitions. */
	for ( int ep = 0; ep < epsilonLinks.length(); ep++ ) {
		/* Get the link. */
		EpsilonLink &link = epsilonLinks[ep];
		NameInst *resolvedName = 0;

		if ( link.target->length() == 1 && link.target->data[0] == "final" ) {
			/* Epsilon drawn to an implicit final state. An implicit final is
			 * only available in join operations. */
			resolvedName = pd->localNameScope->final;
		}
		else {
			/* Do an search for the name. */
			NameSet resolved;
			pd->resolveFrom( resolved, pd->localNameScope, link.target, 0 );
			if ( resolved.length() > 0 ) {
				/* Take the first one. */
				resolvedName = resolved[0];
				if ( resolved.length() > 1 ) {
					/* Complain about the multiple references. */
					error(link.loc) << "state reference " << link.target << 
							" resolves to multiple entry points" << endl;
					errorStateLabels( resolved );
				}
			}
		}

		/* This is tricky, we stuff resolved epsilon transitions into one long
		 * vector in the parse data structure. Since the name resolution and
		 * graph generation both do identical walks of the parse tree we
		 * should always find the link resolutions in the right place.  */
		pd->epsilonResolvedLinks.append( resolvedName );

		if ( resolvedName != 0 ) {
			/* Found the name, bump of the reference count on it. */
			resolvedName->numRefs += 1;
		}
		else {
			/* Complain, no recovery action, the epsilon op will ignore any
			 * epsilon transitions whose names did not resolve. */
			error(link.loc) << "could not resolve label " << link.target << endl;
		}
	}

	if ( labels.length() > 0 )
		pd->popNameScope( nameFrame );
}


/* Clean up after a factor with repetition node. */
FactorWithRep::~FactorWithRep()
{
	switch ( type ) {
		case StarType: case StarStarType: case OptionalType: case PlusType:
		case ExactType: case MaxType: case MinType: case RangeType:
			delete factorWithRep;
		case FactorWithNegType:
			delete factorWithNeg;
			break;
	}
}

void Factor::applyGuardedPrior( ParseData *pd, FsmAp *rtnVal )
{
	priorDescs[0].key = pd->nextPriorKey;
	priorDescs[0].priority = 0;
	priorDescs[0].guarded = true;
	priorDescs[0].guardId = repId;
	priorDescs[0].other = &priorDescs[1];

	priorDescs[1].key = pd->nextPriorKey;
	priorDescs[1].priority = 1;
	priorDescs[1].guarded = true;
	priorDescs[1].guardId = repId;
	priorDescs[1].other = &priorDescs[0];

	/* Roll over for next allocation. */
	pd->nextPriorKey += 1;

	rtnVal->startState->guardedInTable.setPrior( 0, &priorDescs[0] );
}

void Factor::applyGuardedPrior2( ParseData *pd, FsmAp *rtnVal )
{
	priorDescs[2].key = pd->nextPriorKey;
	priorDescs[2].priority = 0;
	priorDescs[2].guarded = true;
	priorDescs[2].guardId = repId;
	priorDescs[2].other = &priorDescs[3];

	priorDescs[3].key = pd->nextPriorKey;
	priorDescs[3].guarded = true;
	priorDescs[3].priority = 1;
	priorDescs[3].guardId = repId;
	priorDescs[3].other = &priorDescs[2];

	/* Roll over for next allocation. */
	pd->nextPriorKey += 1;

	rtnVal->startState->guardedInTable.setPrior( 0, &priorDescs[2] );
	
	rtnVal->allTransPrior( pd->curPriorOrd++, &priorDescs[3] );
	rtnVal->leaveFsmPrior( pd->curPriorOrd++, &priorDescs[2] );

	/* Shift over the start action orders then do the kleene star. */
	pd->curActionOrd += rtnVal->shiftStartActionOrder( pd->curActionOrd );
}

void Factor::condCost( Action *action )
{
	action->costMark = true;
	action->costId = repId;
}

/* Evaluate a factor with repetition node. */
FsmRes FactorWithRep::walk( ParseData *pd )
{
	switch ( type ) {
	case StarType: {
		/* Evaluate the FactorWithRep. */
		FsmRes factorTree = factorWithRep->walk( pd );
		if ( !factorTree.success() )
			return factorTree;
		
		if ( factorTree.fsm->startState->isFinState() ) {
			if ( pd->id->inLibRagel ) {
				delete factorTree.fsm;
				return FsmRes( FsmRes::RepetitionError() );
			}
			warning(loc) << "applying kleene star to a machine that "
					"accepts zero length word" << endl;
			factorTree.fsm->unsetFinState( factorTree.fsm->startState );
		}

		/* Shift over the start action orders then do the kleene star. */
		pd->curActionOrd += factorTree.fsm->shiftStartActionOrder( pd->curActionOrd );

		FsmRes res = FsmAp::starOp( factorTree.fsm );
		if ( !res.success() )
			return res;

		afterOpMinimize( res.fsm );
		return res;
	}
	case StarStarType: {
		/* Evaluate the FactorWithRep. */
		FsmRes factorTree = factorWithRep->walk( pd );
		if ( !factorTree.success() )
			return factorTree;

		if ( factorTree.fsm->startState->isFinState() ) {
			if ( pd->id->inLibRagel ) {
				delete factorTree.fsm;
				return FsmRes( FsmRes::RepetitionError() );
			}
			warning(loc) << "applying kleene star to a machine that "
					"accepts zero length word" << endl;
		}

		/* Set up the prior descs. All gets priority one, whereas leaving gets
		 * priority zero. Make a unique key so that these priorities don't
		 * interfere with any priorities set by the user. */
		priorDescs[0].key = pd->nextPriorKey++;
		priorDescs[0].priority = 1;
		factorTree.fsm->allTransPrior( pd->curPriorOrd++, &priorDescs[0] );

		/* Leaveing gets priority 0. Use same unique key. */
		priorDescs[1].key = priorDescs[0].key;
		priorDescs[1].priority = 0;
		factorTree.fsm->leaveFsmPrior( pd->curPriorOrd++, &priorDescs[1] );

		/* Shift over the start action orders then do the kleene star. */
		pd->curActionOrd += factorTree.fsm->shiftStartActionOrder( pd->curActionOrd );

		FsmRes res = FsmAp::starOp( factorTree.fsm );
		if ( !res.success() )
			return res;

		afterOpMinimize( res.fsm );
		return res;
	}
	case OptionalType: {
		/* Make the null fsm. */
		FsmAp *nu = FsmAp::lambdaFsm( pd->fsmCtx );

		/* Evaluate the FactorWithRep. */
		FsmRes factorTree = factorWithRep->walk( pd );
		if ( !factorTree.success() )
			return factorTree;

		/* Perform the question operator. */
		FsmRes res = FsmAp::unionOp( factorTree.fsm, nu );
		if ( !res.success() )
			return res;

		afterOpMinimize( res.fsm );
		return res;
	}
	case PlusType: {
		/* Evaluate the FactorWithRep. */
		FsmRes factorTree = factorWithRep->walk( pd );
		if ( !factorTree.success() )
			return factorTree;

		if ( factorTree.fsm->startState->isFinState() ) {
			if ( pd->id->inLibRagel ) {
				delete factorTree.fsm;
				return FsmRes( FsmRes::RepetitionError() );
			}
			warning(loc) << "applying plus operator to a machine that "
					"accepts zero length word" << endl;
		}

		/* Need a duplicate for the star end. */
		FsmAp *factorDup = new FsmAp( *factorTree.fsm );

		/* The start func orders need to be shifted before doing the star. */
		pd->curActionOrd += factorDup->shiftStartActionOrder( pd->curActionOrd );

		/* Star the duplicate. */
		FsmRes res1 = FsmAp::starOp( factorDup );
		if ( !res1.success() )
			return res1;

		afterOpMinimize( res1.fsm );

		FsmRes res2 = FsmAp::concatOp( factorTree.fsm, res1.fsm );
		if ( !res2.success() )
			return res2;

		afterOpMinimize( res2.fsm );
		return res2;
	}
	case ExactType: {
		/* Get an int from the repetition amount. */
		if ( lowerRep == 0 ) {
			/* No copies. Don't need to evaluate the factorWithRep. 
			 * This Defeats the purpose so give a warning. */
			if ( pd->id->inLibRagel )
				return FsmRes( FsmRes::RepetitionError() );

			warning(loc) << "exactly zero repetitions results "
					"in the null machine" << endl;

			return FsmRes( FsmRes::Fsm(), FsmAp::lambdaFsm( pd->fsmCtx ) );
		}
		else {
			/* Evaluate the first FactorWithRep. */
			FsmRes factorTree = factorWithRep->walk( pd );
			if ( !factorTree.success() )
				return factorTree;

			if ( factorTree.fsm->startState->isFinState() ) {
				if ( pd->id->inLibRagel ) {
					delete factorTree.fsm;
					return FsmRes( FsmRes::RepetitionError() );
				}
				warning(loc) << "applying repetition to a machine that "
						"accepts zero length word" << endl;
			}

			/* The start func orders need to be shifted before doing the
			 * repetition. */
			pd->curActionOrd += factorTree.fsm->shiftStartActionOrder( pd->curActionOrd );

			/* Do the repetition on the machine. Already guarded against n == 0 */
			FsmRes res = FsmAp::repeatOp( factorTree.fsm, lowerRep );
			if ( !res.success() )
				return res;

			afterOpMinimize( res.fsm );
			return res;
		}
		break;
	}
	case MaxType: {
		/* Get an int from the repetition amount. */
		if ( upperRep == 0 ) {
			/* No copies. Don't need to evaluate the factorWithRep. 
			 * This Defeats the purpose so give a warning. */
			if ( pd->id->inLibRagel )
				return FsmRes( FsmRes::RepetitionError() );

			warning(loc) << "max zero repetitions results "
					"in the null machine" << endl;

			return FsmRes( FsmRes::Fsm(), FsmAp::lambdaFsm( pd->fsmCtx ) );
		}
		else {
			/* Evaluate the first FactorWithRep. */
			FsmRes factorTree = factorWithRep->walk( pd );
			if ( !factorTree.success() )
				return factorTree;

			if ( factorTree.fsm->startState->isFinState() ) {
				if ( pd->id->inLibRagel ) {
					delete factorTree.fsm;
					return FsmRes( FsmRes::RepetitionError() );
				}
				warning(loc) << "applying max repetition to a machine that "
						"accepts zero length word" << endl;
			}

			/* The start func orders need to be shifted before doing the 
			 * repetition. */
			pd->curActionOrd += factorTree.fsm->shiftStartActionOrder( pd->curActionOrd );

			/* Do the repetition on the machine. Already guarded against n == 0 */
			FsmRes res = FsmAp::optionalRepeatOp( factorTree.fsm, upperRep );
			if ( !res.success() )
				return res;

			afterOpMinimize( res.fsm );
			return res;
		}
		break;
	}
	case MinType: {
		/* Evaluate the repeated machine. */
		FsmRes factorTree = factorWithRep->walk( pd );
		if ( !factorTree.success() )
			return factorTree;

		if ( factorTree.fsm->startState->isFinState() ) {
			if ( pd->id->inLibRagel ) {
				delete factorTree.fsm;
				return FsmRes( FsmRes::RepetitionError() );
			}
			warning(loc) << "applying min repetition to a machine that "
					"accepts zero length word" << endl;
		}

		/* The start func orders need to be shifted before doing the repetition
		 * and the kleene star. */
		pd->curActionOrd += factorTree.fsm->shiftStartActionOrder( pd->curActionOrd );
	
		if ( lowerRep == 0 ) {
			/* Acts just like a star op on the machine to return. */
			FsmRes res = FsmAp::starOp( factorTree.fsm );
			if ( !res.success() )
				return res;

			afterOpMinimize( res.fsm );
			return res;
		}
		else {
			/* Take a duplicate for the plus. */
			FsmAp *dup = new FsmAp( *factorTree.fsm );

			/* Do repetition on the first half. */
			FsmRes res1 = FsmAp::repeatOp( factorTree.fsm, lowerRep );
			if ( !res1.success() )
				return res1;

			afterOpMinimize( res1.fsm );

			/* Star the duplicate. */
			FsmRes res2 = FsmAp::starOp( dup );
			if ( !res2.success() )
				return res2;

			afterOpMinimize( res2.fsm );

			/* Tack on the kleene star. */
			FsmRes res3 = FsmAp::concatOp( res1.fsm, res2.fsm );
			if ( !res3.success() )
				return res3;

			afterOpMinimize( res3.fsm );
			return res3;
		}
		break;
	}
	case RangeType: {
		/* Check for bogus range. */
		if ( upperRep - lowerRep < 0 ) {
			error(loc) << "invalid range repetition" << endl;

			/* Return null machine as recovery. */
			return FsmRes( FsmRes::Fsm(), FsmAp::lambdaFsm( pd->fsmCtx ) );
		}
		else if ( lowerRep == 0 && upperRep == 0 ) {
			/* No copies. Don't need to evaluate the factorWithRep.  This
			 * defeats the purpose so give a warning. */
			if ( pd->id->inLibRagel )
				return FsmRes( FsmRes::RepetitionError() );

			warning(loc) << "zero to zero repetitions results "
					"in the null machine" << endl;

			return FsmRes( FsmRes::Fsm(), FsmAp::lambdaFsm( pd->fsmCtx ) );
		}
		else {
			/* Now need to evaluate the repeated machine. */
			FsmRes factorTree = factorWithRep->walk( pd );
			if ( !factorTree.success() )
				return factorTree;

			if ( factorTree.fsm->startState->isFinState() ) {
				if ( pd->id->inLibRagel ) {
					delete factorTree.fsm;
					return FsmRes( FsmRes::RepetitionError() );
				}
				warning(loc) << "applying range repetition to a machine that "
						"accepts zero length word" << endl;
			}

			/* The start func orders need to be shifted before doing both kinds
			 * of repetition. */
			pd->curActionOrd += factorTree.fsm->shiftStartActionOrder( pd->curActionOrd );

			if ( lowerRep == 0 ) {
				/* Just doing max repetition. Already guarded against n == 0. */
				FsmRes res = FsmAp::optionalRepeatOp( factorTree.fsm, upperRep );
				if ( !res.success() )
					return res;

				afterOpMinimize( res.fsm );
				return res;
			}
			else if ( lowerRep == upperRep ) {
				/* Just doing exact repetition. Already guarded against n == 0. */
				FsmRes res = FsmAp::repeatOp( factorTree.fsm, lowerRep );
				if ( !res.success() )
					return res;

				afterOpMinimize( res.fsm );
				return res;
			}
			else {
				/* This is the case that 0 < lowerRep < upperRep. Take a
				 * duplicate for the optional repeat. */
				FsmAp *dup = new FsmAp( *factorTree.fsm );

				/* Do repetition on the first half. */
				FsmRes res1 = FsmAp::repeatOp( factorTree.fsm, lowerRep );
				if ( !res1.success() )
					return res1;

				afterOpMinimize( res1.fsm );

				/* Do optional repetition on the second half. */
				FsmRes res2 = FsmAp::optionalRepeatOp( dup, upperRep - lowerRep );
				if ( !res2.success() )
					return res2;

				afterOpMinimize( res2.fsm );

				/* Tak on the duplicate machine. */
				FsmRes res3 = FsmAp::concatOp( res1.fsm, res2.fsm );
				if ( !res3.success() )
					return res3;

				afterOpMinimize( res3.fsm );
				return res3;
			}
		}
		break;
	}
	case FactorWithNegType: {
		/* Evaluate the Factor. Pass it up. */
		return factorWithNeg->walk( pd );
	}}
	return FsmRes( FsmRes::Aborted() );
}

void FactorWithRep::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case StarType:
	case StarStarType:
	case OptionalType:
	case PlusType:
	case ExactType:
	case MaxType:
	case MinType:
	case RangeType:
		factorWithRep->makeNameTree( pd );
		break;
	case FactorWithNegType:
		factorWithNeg->makeNameTree( pd );
		break;
	}
}

void FactorWithRep::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case StarType:
	case StarStarType:
	case OptionalType:
	case PlusType:
	case ExactType:
	case MaxType:
	case MinType:
	case RangeType:
		factorWithRep->resolveNameRefs( pd );
		break;
	case FactorWithNegType:
		factorWithNeg->resolveNameRefs( pd );
		break;
	}
}

/* Clean up after a factor with negation node. */
FactorWithNeg::~FactorWithNeg()
{
	switch ( type ) {
		case NegateType:
		case CharNegateType:
			delete factorWithNeg;
			break;
		case FactorType:
			delete factor;
			break;
	}
}

/* Evaluate a factor with negation node. */
FsmRes FactorWithNeg::walk( ParseData *pd )
{
	switch ( type ) {
	case NegateType: {
		/* Evaluate the factorWithNeg. */
		FsmRes toNegate = factorWithNeg->walk( pd );

		/* Negation is subtract from dot-star. */
		FsmAp *ds = dotStarFsm( pd );
		FsmRes res = FsmAp::subtractOp( ds, toNegate.fsm );

		afterOpMinimize( res.fsm );
		return res;
	}
	case CharNegateType: {
		/* Evaluate the factorWithNeg. */
		FsmRes toNegate = factorWithNeg->walk( pd );

		/* CharNegation is subtract from dot. */
		FsmAp *ds = dotFsm( pd );
		FsmRes res = FsmAp::subtractOp( ds, toNegate.fsm );

		afterOpMinimize( res.fsm );
		return res;
	}
	case FactorType: {
		/* Evaluate the Factor. Pass it up. */
		return factor->walk( pd );
	}}
	return FsmRes( FsmRes::Aborted() );
}

void FactorWithNeg::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case NegateType:
	case CharNegateType:
		factorWithNeg->makeNameTree( pd );
		break;
	case FactorType:
		factor->makeNameTree( pd );
		break;
	}
}

void FactorWithNeg::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case NegateType:
	case CharNegateType:
		factorWithNeg->resolveNameRefs( pd );
		break;
	case FactorType:
		factor->resolveNameRefs( pd );
		break;
	}
}

/* Clean up after a factor node. */
Factor::~Factor()
{
	switch ( type ) {
		case LiteralType:
			delete literal;
			break;
		case RangeType:
			delete range;
			break;
		case OrExprType:
			delete reItem;
			break;
		case RegExprType:
			delete regExpr;
			break;
		case ReferenceType:
			break;
		case ParenType:
			delete join;
			break;
		case LongestMatchType:
			delete longestMatch;
			break;
		case NfaRep: case CondStar: case CondPlus:
			delete expression;
			break;
	}
}

FsmRes Factor::condPlus( ParseData *pd )
{
	Action *ini = action1;
	Action *inc = action2;
	Action *min = action3;
	Action *max = action4;

	condCost( ini );
	condCost( inc );
	condCost( min );
	if ( max != 0 )
		condCost( max );

	FsmRes exprTree = expression->walk( pd );
	if ( !exprTree.success() )
		return exprTree;

	exprTree.fsm->startFsmAction( 0, inc );
	afterOpMinimize( exprTree.fsm );

	if ( max != 0 ) {
		FsmRes res = exprTree.fsm->startFsmCondition( max, true );
		if ( !res.success() )
			return res;

		afterOpMinimize( exprTree.fsm );
	}

	/* Plus Operation. */
	if ( exprTree.fsm->startState->isFinState() ) {
		if ( pd->id->inLibRagel ) {
			delete exprTree.fsm;
			return FsmRes( FsmRes::RepetitionError() );
		}
		warning(loc) << "applying plus operator to a machine that "
				"accepts zero length word" << endl;
	}

	/* Need a duplicated for the star end. */
	FsmAp *dup = new FsmAp( *exprTree.fsm );

	/* The start func orders need to be shifted before doing the star. */
	pd->curActionOrd += dup->shiftStartActionOrder( pd->curActionOrd );

	applyGuardedPrior2( pd, dup );

	/* Star the duplicate. */
	FsmRes res1 = FsmAp::starOp( dup );
	if ( !res1.success() )
		return res1;

	afterOpMinimize( res1.fsm );

	FsmRes res2 = FsmAp::concatOp( exprTree.fsm, res1.fsm );
	if ( !res2.success() )
		return res2;

	afterOpMinimize( res2.fsm );

	/* End plus operation. */

	res2.fsm->leaveFsmCondition( min, true );

	/* Init action. */
	res2.fsm->startFromStateAction( 0,  ini );

	/* Leading priority guard. */
	applyGuardedPrior( pd, res2.fsm );

	return res2;
}

FsmRes Factor::condStar( ParseData *pd )
{
	Action *min = action3;

	FsmRes cp = condPlus( pd );
	if ( !cp.success() )
		return cp;

	StateAp *newStart = cp.fsm->dupStartState();
	cp.fsm->unsetStartState();
	cp.fsm->setStartState( newStart );

	/* Now ensure the new start state is a final state. */
	cp.fsm->setFinState( newStart );
	cp.fsm->addOutCondition( newStart, min, true );

	return cp;
}

/* Evaluate a factor node. */
FsmRes Factor::walk( ParseData *pd )
{
	switch ( type ) {
	case LiteralType:
		return FsmRes( FsmRes::Fsm(), literal->walk( pd ) );
	case RangeType:
		return FsmRes( FsmRes::Fsm(), range->walk( pd ) );
	case OrExprType:
		return reItem->walk( pd, 0 );
	case RegExprType:
		return FsmRes( FsmRes::Fsm(), regExpr->walk( pd, 0 ) );
	case ReferenceType:
		return varDef->walk( pd );
	case ParenType:
		return join->walk( pd );
	case LongestMatchType:
		return longestMatch->walk( pd );
	case NfaRep: {
		FsmRes exprTree = expression->walk( pd );

		FsmRes res = FsmAp::nfaRepeatOp( exprTree.fsm, action1, action2, action3,
				action4, action5, action6, pd->curActionOrd );

		res.fsm->verifyIntegrity();
		return res;
	}
	case CondStar:
		return condStar( pd );
	case CondPlus:
		return condPlus( pd );
	}

	return FsmRes( FsmRes::Aborted() );
}

void Factor::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case LiteralType:
	case RangeType:
	case OrExprType:
	case RegExprType:
		break;
	case ReferenceType:
		varDef->makeNameTree( loc, pd );
		break;
	case ParenType:
		join->makeNameTree( pd );
		break;
	case LongestMatchType:
		longestMatch->makeNameTree( pd );
		break;
	case NfaRep:
	case CondStar:
	case CondPlus:
		expression->makeNameTree( pd );
		break;
	}
}

void Factor::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case LiteralType:
	case RangeType:
	case OrExprType:
	case RegExprType:
		break;
	case ReferenceType:
		varDef->resolveNameRefs( pd );
		break;
	case ParenType:
		join->resolveNameRefs( pd );
		break;
	case LongestMatchType:
		longestMatch->resolveNameRefs( pd );
		break;
	case NfaRep:
	case CondStar:
	case CondPlus:
		expression->resolveNameRefs( pd );
		break;
	}
}

/* Clean up a range object. Must delete the two literals. */
Range::~Range()
{
	delete lowerLit;
	delete upperLit;
}

/* Evaluate a range. Gets the lower an upper key and makes an fsm range. */
FsmAp *Range::walk( ParseData *pd )
{
	/* Construct and verify the suitability of the lower end of the range. */
	FsmAp *lowerFsm = lowerLit->walk( pd );
	if ( !lowerFsm->checkSingleCharMachine() ) {
		error(lowerLit->loc) << 
			"bad range lower end, must be a single character" << endl;
	}

	/* Construct and verify the upper end. */
	FsmAp *upperFsm = upperLit->walk( pd );
	if ( !upperFsm->checkSingleCharMachine() ) {
		error(upperLit->loc) << 
			"bad range upper end, must be a single character" << endl;
	}

	/* Grab the keys from the machines, then delete them. */
	Key lowKey = lowerFsm->startState->outList.head->lowKey;
	Key highKey = upperFsm->startState->outList.head->lowKey;
	delete lowerFsm;
	delete upperFsm;

	/* Validate the range. */
	if ( pd->fsmCtx->keyOps->gt( lowKey, highKey ) ) {
		/* Recover by setting upper to lower; */
		error(lowerLit->loc) << "lower end of range is greater then upper end" << endl;
		highKey = lowKey;
	}

	/* Return the range now that it is validated. */
	FsmAp *retFsm = FsmAp::rangeFsm( pd->fsmCtx, lowKey, highKey );

	/* If case independent, then union the portion that covers alphas. */
	if ( caseIndep ) { 
		if ( lowKey.getVal() <= 'z' ) {
			int low, high;
			if ( lowKey.getVal() <= 'a' )
				low = 'a';
			else
				low = lowKey.getVal();

			if ( highKey.getVal() >= 'a' ) {
				if ( highKey.getVal() >= 'z' )
					high = 'z';
				else
					high = highKey.getVal();

				/* Add in upper(low) .. upper(high) */

				FsmAp *addFsm = FsmAp::rangeFsm( pd->fsmCtx, toupper(low), toupper(high) );
				FsmRes res = FsmAp::unionOp( retFsm, addFsm );
				retFsm = res.fsm;
			}
		}

		if ( lowKey.getVal() <= 'Z' ) {
			int low, high;
			if ( lowKey.getVal() <= 'A' )
				low = 'A';
			else
				low = lowKey.getVal();

			if ( highKey.getVal() >= 'A' ) {
				if ( highKey.getVal() >= 'Z' )
					high = 'Z';
				else
					high = highKey.getVal();

				/* Add in lower(low) .. lower(high) */
				FsmAp *addFsm = FsmAp::rangeFsm( pd->fsmCtx, tolower(low), tolower(high) );
				FsmRes res = FsmAp::unionOp( retFsm, addFsm );
				retFsm = res.fsm;
			}
		}
	}
	return retFsm;
}

/* Evaluate a literal object. */
FsmAp *Literal::walk( ParseData *pd )
{
	/* FsmAp to return, is the alphabet signed. */
	FsmAp *rtnVal = 0;

	switch ( type ) {
	case Number: {
		/* Make a C string. Maybe put - up front. */
		Vector<char> num = data;
		if ( neg )
			num.insert( 0, '-' );
		num.append( 0 );

		/* Make the fsm key in int format. */
		Key fsmKey = makeFsmKeyNum( num.data, loc, pd );

		/* Make the new machine. */
		rtnVal = FsmAp::concatFsm( pd->fsmCtx, fsmKey );
		break;
	}
	case LitString: {
		/* Make the array of keys in int format. */
		long length;
		bool caseInsensitive;
		char *litstr = prepareLitString( loc, data.data, data.length(), 
				length, caseInsensitive );
		Key *arr = new Key[length];
		makeFsmKeyArray( arr, litstr, length, pd );

		/* Make the new machine. */
		if ( caseInsensitive )
			rtnVal = FsmAp::concatFsmCI( pd->fsmCtx, arr, length );
		else
			rtnVal = FsmAp::concatFsm( pd->fsmCtx, arr, length );
		delete[] litstr;
		delete[] arr;
		break;
	}}
	return rtnVal;
}

/* Clean up after a regular expression object. */
RegExpr::~RegExpr()
{
	switch ( type ) {
		case RecurseItem:
			delete regExpr;
			delete item;
			break;
		case Empty:
			break;
	}
}

/* Evaluate a regular expression object. */
FsmAp *RegExpr::walk( ParseData *pd, RegExpr *rootRegex )
{
	/* This is the root regex, pass down a pointer to this. */
	if ( rootRegex == 0 )
		rootRegex = this;

	FsmAp *rtnVal = 0;
	switch ( type ) {
		case RecurseItem: {
			/* Walk both items. */
			rtnVal = regExpr->walk( pd, rootRegex );
			FsmRes fsm2 = item->walk( pd, rootRegex );
			FsmRes res = FsmAp::concatOp( rtnVal, fsm2.fsm );
			rtnVal = res.fsm;
			break;
		}
		case Empty: {
			rtnVal = FsmAp::lambdaFsm( pd->fsmCtx );
			break;
		}
	}
	return rtnVal;
}

/* Clean up after an item in a regular expression. */
ReItem::~ReItem()
{
	switch ( type ) {
		case Data:
		case Dot:
			break;
		case OrBlock:
		case NegOrBlock:
			delete orBlock;
			break;
	}
}

/* Evaluate a regular expression object. */
FsmRes ReItem::walk( ParseData *pd, RegExpr *rootRegex )
{
	/* The fsm to return, is the alphabet signed? */
	FsmAp *rtnVal = 0;

	switch ( type ) {
		case Data: {
			/* Move the data into an integer array and make a concat fsm. */
			Key *arr = new Key[data.length()];
			makeFsmKeyArray( arr, data.data, data.length(), pd );

			/* Make the concat fsm. */
			if ( rootRegex != 0 && rootRegex->caseInsensitive )
				rtnVal = FsmAp::concatFsmCI( pd->fsmCtx, arr, data.length() );
			else
				rtnVal = FsmAp::concatFsm( pd->fsmCtx, arr, data.length() );
			delete[] arr;
			break;
		}
		case Dot: {
			/* Make the dot fsm. */
			rtnVal = dotFsm( pd );
			break;
		}
		case OrBlock: {
			/* Get the or block and minmize it. */
			rtnVal = orBlock->walk( pd, rootRegex );
			if ( rtnVal == 0 )
				rtnVal = FsmAp::lambdaFsm( pd->fsmCtx );
			rtnVal->minimizePartition2();
			break;
		}
		case NegOrBlock: {
			/* Get the or block and minimize it. */
			FsmAp *fsm = orBlock->walk( pd, rootRegex );
			fsm->minimizePartition2();

			/* Make a dot fsm and subtract from it. */
			rtnVal = dotFsm( pd );
			FsmRes res = FsmAp::subtractOp( rtnVal, fsm );
			rtnVal = res.fsm;
			rtnVal->minimizePartition2();
			break;
		}
	}

	/* If the item is followed by a star, then apply the star op. */
	if ( star ) {
		if ( rtnVal->startState->isFinState() ) {
			if ( pd->id->inLibRagel ) {
				delete rtnVal;
				return FsmRes( FsmRes::RepetitionError() );
			}

			warning(loc) << "applying kleene star to a machine that "
					"accepts zero length word" << endl;
		}

		FsmRes res = FsmAp::starOp( rtnVal );
		rtnVal = res.fsm;
		rtnVal->minimizePartition2();
	}

	return FsmRes( FsmRes::Fsm(), rtnVal );
}

/* Clean up after an or block of a regular expression. */
ReOrBlock::~ReOrBlock()
{
	switch ( type ) {
		case RecurseItem:
			delete orBlock;
			delete item;
			break;
		case Empty:
			break;
	}
}


/* Evaluate an or block of a regular expression. */
FsmAp *ReOrBlock::walk( ParseData *pd, RegExpr *rootRegex )
{
	FsmAp *rtnVal = 0;
	switch ( type ) {
		case RecurseItem: {
			/* Evaluate the two fsm. */
			FsmAp *fsm1 = orBlock->walk( pd, rootRegex );
			FsmAp *fsm2 = item->walk( pd, rootRegex );
			if ( fsm1 == 0 )
				rtnVal = fsm2;
			else {
				FsmRes res = FsmAp::unionOp( fsm1, fsm2 );
				fsm1 = res.fsm;
				rtnVal = fsm1;
			}
			break;
		}
		case Empty: {
			rtnVal = 0;
			break;
		}
	}
	return rtnVal;;
}

/* Evaluate an or block item of a regular expression. */
FsmAp *ReOrItem::walk( ParseData *pd, RegExpr *rootRegex )
{
	KeyOps *keyOps = pd->fsmCtx->keyOps;

	/* The return value, is the alphabet signed? */
	FsmAp *rtnVal = 0;
	switch ( type ) {
	case Data: {
		/* Put the or data into an array of ints. Note that we find unique
		 * keys. Duplicates are silently ignored. The alternative would be to
		 * issue warning or an error but since we can't with [a0-9a] or 'a' |
		 * 'a' don't bother here. */
		KeySet keySet( keyOps );
		makeFsmUniqueKeyArray( keySet, data.data, data.length(), 
			rootRegex != 0 ? rootRegex->caseInsensitive : false, pd );

		/* Run the or operator. */
		rtnVal = FsmAp::orFsm( pd->fsmCtx, keySet.data, keySet.length() );
		break;
	}
	case Range: {
		/* Make the upper and lower keys. */
		Key lowKey = makeFsmKeyChar( lower, pd );
		Key highKey = makeFsmKeyChar( upper, pd );

		/* Validate the range. */
		if ( keyOps->gt( lowKey, highKey ) ) {
			/* Recover by setting upper to lower; */
			error(loc) << "lower end of range is greater then upper end" << endl;
			highKey = lowKey;
		}

		/* Make the range machine. */
		rtnVal = FsmAp::rangeFsm( pd->fsmCtx, lowKey, highKey );

		if ( rootRegex != 0 && rootRegex->caseInsensitive ) {
			if ( keyOps->le( lowKey, 'Z' ) && pd->fsmCtx->keyOps->le( 'A', highKey ) ) {
				Key otherLow = keyOps->lt( lowKey, 'A' ) ? Key('A') : lowKey;
				Key otherHigh = keyOps->lt( 'Z', highKey ) ? Key('Z') : highKey;

				otherLow = keyOps->add( 'a', ( keyOps->sub( otherLow, 'A' ) ) );
				otherHigh = keyOps->add( 'a', ( keyOps->sub( otherHigh, 'A' ) ) );

				FsmAp *otherRange = FsmAp::rangeFsm( pd->fsmCtx, otherLow, otherHigh );
				FsmRes res = FsmAp::unionOp( rtnVal, otherRange );
				rtnVal = res.fsm;
				rtnVal->minimizePartition2();
			}
			else if ( keyOps->le( lowKey, 'z' ) && keyOps->le( 'a', highKey ) ) {
				Key otherLow = keyOps->lt( lowKey, 'a' ) ? Key('a') : lowKey;
				Key otherHigh = keyOps->lt( 'z', highKey ) ? Key('z') : highKey;

				otherLow = keyOps->add('A' , ( keyOps->sub( otherLow , 'a' ) ));
				otherHigh = keyOps->add('A' , ( keyOps->sub( otherHigh , 'a' ) ));

				FsmAp *otherRange = FsmAp::rangeFsm( pd->fsmCtx, otherLow, otherHigh );
				FsmRes res = FsmAp::unionOp( rtnVal, otherRange );
				rtnVal = res.fsm;
				rtnVal->minimizePartition2();
			}
		}

		break;
	}}
	return rtnVal;
}
