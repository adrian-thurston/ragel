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

#include "lmparse.h"
#include "parsetree.h"
#include "input.h"
#include "fsmrun.h"

#include <iostream>
#include <iomanip>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>


using namespace std;
ostream &operator<<( ostream &out, const NameRef &nameRef );
ostream &operator<<( ostream &out, const NameInst &nameInst );
ostream &operator<<( ostream &out, const Token &token );

/* Convert the literal string which comes in from the scanner into an array of
 * characters with escapes and options interpreted. Also null terminates the
 * string. Though this null termination should not be relied on for
 * interpreting literals in the parser because the string may contain a
 * literal string with \0 */
void prepareLitString( String &result, bool &caseInsensitive, 
		const String &srcString, const InputLoc &loc )
{
	result.setAs( String::Fresh(), srcString.length() );
	caseInsensitive = false;

	char *src = srcString.data + 1;
	char *end = srcString.data + srcString.length() - 1;

	while ( *end != '\'' && *end != '\"' && *end != '\n' ) {
		if ( *end == 'i' )
			caseInsensitive = true;
		else {
			error( loc ) << "literal string '" << *end << 
					"' option not supported" << endl;
		}
		end -= 1;
	}

	if ( *end == '\n' )
		end++;

	char *dest = result.data;
	int len = 0;
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

	result.chop( len );
}

int CmpUniqueType::compare( const UniqueType &ut1, const UniqueType &ut2 )
{
	if ( ut1.typeId < ut2.typeId )
		return -1;
	else if ( ut1.typeId > ut2.typeId )
		return 1;
	else if ( ut1.typeId == TYPE_TREE || 
			ut1.typeId == TYPE_PTR || 
			ut1.typeId == TYPE_REF )
	{
		if ( ut1.langEl < ut2.langEl )
			return -1;
		else if ( ut1.langEl > ut2.langEl )
			return 1;
	}
	else if ( ut1.typeId == TYPE_ITER ) {
		if ( ut1.iterDef < ut2.iterDef )
			return -1;
		else if ( ut1.iterDef > ut2.iterDef )
			return 1;
	}
	else {
		/* Fail on anything unimplemented. */
		assert( false );
	}

	return 0;
}

int CmpUniqueRepeat::compare( const UniqueRepeat &ut1, const UniqueRepeat &ut2 )
{
	if ( ut1.repeatType < ut2.repeatType )
		return -1;
	else if ( ut1.repeatType > ut2.repeatType )
		return 1;
	else {
		if ( ut1.langEl < ut2.langEl )
			return -1;
		else if ( ut1.langEl > ut2.langEl )
			return 1;
	}

	return 0;
}

int CmpUniqueMap::compare( const UniqueMap &ut1, const UniqueMap &ut2 )
{
	if ( ut1.key < ut2.key )
		return -1;
	else if ( ut1.key > ut2.key )
		return 1;
	else {
		if ( ut1.value < ut2.value )
			return -1;
		else if ( ut1.value > ut2.value )
			return 1;
	}

	return 0;
}

int CmpUniqueList::compare( const UniqueList &ut1, const UniqueList &ut2 )
{
	if ( ut1.value < ut2.value )
		return -1;
	else if ( ut1.value > ut2.value )
		return 1;

	return 0;
}

int CmpUniqueVector::compare( const UniqueVector &ut1, const UniqueVector &ut2 )
{
	if ( ut1.value < ut2.value )
		return -1;
	else if ( ut1.value > ut2.value )
		return 1;

	return 0;
}

int CmpUniqueParser::compare( const UniqueParser &ut1, const UniqueParser &ut2 )
{
	if ( ut1.parseType < ut2.parseType )
		return -1;
	else if ( ut1.parseType > ut2.parseType )
		return 1;

	return 0;
}

FsmGraph *VarDef::walk( Compiler *pd )
{
	/* We enter into a new name scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Recurse on the expression. */
	FsmGraph *rtnVal = join->walk( pd );
	
	/* Do the tranfer of local error actions. */
	LocalErrDictEl *localErrDictEl = pd->localErrDict.find( name );
	if ( localErrDictEl != 0 ) {
		for ( StateList::Iter state = rtnVal->stateList; state.lte(); state++ )
			rtnVal->transferErrorActions( state, localErrDictEl->value );
	}

	/* If the expression below is a join operation with multiple expressions
	 * then it just had epsilon transisions resolved. If it is a join
	 * with only a single expression then run the epsilon op now. */
	if ( join->exprList.length() == 1 )
		rtnVal->epsilonOp();

	/* We can now unset entry points that are not longer used. */
	pd->unsetObsoleteEntries( rtnVal );

	/* If the name of the variable is referenced then add the entry point to
	 * the graph. */
	if ( pd->curNameInst->numRefs > 0 )
		rtnVal->setEntry( pd->curNameInst->id, rtnVal->startState );
	
	/* Pop the name scope. */
	pd->popNameScope( nameFrame );
	return rtnVal;
}

void VarDef::makeNameTree( const InputLoc &loc, Compiler *pd )
{
	/* The variable definition enters a new scope. */
	NameInst *prevNameInst = pd->curNameInst;
	pd->curNameInst = pd->addNameInst( loc, name, false );

	/* Recurse. */
	join->makeNameTree( pd );

	/* The name scope ends, pop the name instantiation. */
	pd->curNameInst = prevNameInst;
}

FsmGraph *RegionDef::walk( Compiler *pd )
{
	/* We enter into a new name scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Recurse on the expression. */
	FsmGraph *rtnVal = tokenRegion->walk( pd );
	
	/* Do the tranfer of local error actions. */
	LocalErrDictEl *localErrDictEl = pd->localErrDict.find( name );
	if ( localErrDictEl != 0 ) {
		for ( StateList::Iter state = rtnVal->stateList; state.lte(); state++ )
			rtnVal->transferErrorActions( state, localErrDictEl->value );
	}

	/* We can now unset entry points that are not longer used. */
	pd->unsetObsoleteEntries( rtnVal );

	/* If the name of the variable is referenced then add the entry point to
	 * the graph. */
	if ( pd->curNameInst->numRefs > 0 )
		rtnVal->setEntry( pd->curNameInst->id, rtnVal->startState );
	
	/* Pop the name scope. */
	pd->popNameScope( nameFrame );
	return rtnVal;
}

void RegionDef::makeNameTree( const InputLoc &loc, Compiler *pd )
{
	/* The variable definition enters a new scope. */
	NameInst *prevNameInst = pd->curNameInst;
	pd->curNameInst = pd->addNameInst( loc, name, false );

	pd->curNameInst->isLongestMatch = true;

	/* Recurse. */
	tokenRegion->makeNameTree( pd );

	/* Guess we do this now. */
	tokenRegion->makeActions( pd );

	/* The name scope ends, pop the name instantiation. */
	pd->curNameInst = prevNameInst;
}

InputLoc TokenDef::getLoc()
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

Action *TokenRegion::newAction( Compiler *pd, const InputLoc &loc, 
		const String &name, InlineList *inlineList )
{
	Action *action = new Action( loc, name, inlineList );
	pd->actionList.append( action );
	action->isLmAction = true;
	return action;
}

void TokenRegion::makeActions( Compiler *pd )
{
	/* Make actions that set the action id. */
	for ( TokenDefListReg::Iter lmi = tokenDefList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmSetActId ) );
		char *actName = new char[50];
		sprintf( actName, "store%i", lmi->longestMatchId );
		lmi->setActId = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart on the last character. */
	for ( TokenDefListReg::Iter lmi = tokenDefList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnLast ) );
		char *actName = new char[50];
		sprintf( actName, "imm%i", lmi->longestMatchId );
		lmi->actOnLast = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart on the next
	 * character.  These actions will set tokend themselves (it is the current
	 * char). */
	for ( TokenDefListReg::Iter lmi = tokenDefList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnNext ) );
		char *actName = new char[50];
		sprintf( actName, "lagh%i", lmi->longestMatchId );
		lmi->actOnNext = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart at tokend. These
	 * actions execute some time after matching the last char. */
	for ( TokenDefListReg::Iter lmi = tokenDefList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnLagBehind ) );
		char *actName = new char[50];
		sprintf( actName, "lag%i", lmi->longestMatchId );
		lmi->actLagBehind = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	InputLoc loc;
	loc.line = 1;
	loc.col = 1;

	/* Create the error action. */
	InlineList *il6 = new InlineList;
	il6->append( new InlineItem( loc, this, 0, InlineItem::LmSwitch ) );
	lmActSelect = newAction( pd, loc, "lagsel", il6 );
}

void TokenRegion::makeNameTree( Compiler *pd )
{
	/* Create an anonymous scope for the longest match. Will be used for
	 * restarting machine after matching a token. */
	NameInst *prevNameInst = pd->curNameInst;
	pd->curNameInst = pd->addNameInst( loc, 0, false );

	/* Save off the name inst into the token region. This is only legal for
	 * token regions because they are only ever referenced once (near the root
	 * of the name tree). They cannot have more than one corresponding name
	 * inst. */
	assert( regionNameInst == 0 );
	regionNameInst = pd->curNameInst;

	/* Recurse into all parts of the longest match operator. */
	for ( TokenDefListReg::Iter td = tokenDefList; td.lte(); td++ ) {
		/* Watch out for patternless tokens. */
		if ( td->join != 0 ) 
			td->join->makeNameTree( pd );
	}

	/* The name scope ends, pop the name instantiation. */
	pd->curNameInst = prevNameInst;
}


void TokenRegion::restart( FsmGraph *graph, FsmTrans *trans )
{
	FsmState *fromState = trans->fromState;
	graph->detachTrans( fromState, trans->toState, trans );
	graph->attachTrans( fromState, graph->startState, trans );
}

void TokenRegion::runLongestMatch( Compiler *pd, FsmGraph *graph )
{
	graph->markReachableFromHereStopFinal( graph->startState );
	for ( StateList::Iter ms = graph->stateList; ms.lte(); ms++ ) {
		if ( ms->stateBits & SB_ISMARKED ) {
			ms->lmItemSet.insert( 0 );
			ms->stateBits &= ~ SB_ISMARKED;
		}
	}

	/* Transfer the first item of non-empty lmAction tables to the item sets
	 * of the states that follow. Exclude states that have no transitions out.
	 * This must happen on a separate pass so that on each iteration of the
	 * next pass we have the item set entries from all lmAction tables. */
	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
			if ( trans->lmActionTable.length() > 0 ) {
				LmActionTableEl *lmAct = trans->lmActionTable.data;
				FsmState *toState = trans->toState;
				assert( toState );

				/* Check if there are transitions out, this may be a very
				 * close approximation? Out transitions going nowhere?
				 * FIXME: Check. */
				if ( toState->outList.length() > 0 ) {
					/* Fill the item sets. */
					graph->markReachableFromHereStopFinal( toState );
					for ( StateList::Iter ms = graph->stateList; ms.lte(); ms++ ) {
						if ( ms->stateBits & SB_ISMARKED ) {
							ms->lmItemSet.insert( lmAct->value );
							ms->stateBits &= ~ SB_ISMARKED;
						}
					}
				}
			}
		}
	}

	/* The lmItem sets are now filled, telling us which longest match rules
	 * can succeed in which states. First determine if we need to make sure
	 * act is defaulted to zero. */
	int maxItemSetLength = 0;
	graph->markReachableFromHereStopFinal( graph->startState );
	for ( StateList::Iter ms = graph->stateList; ms.lte(); ms++ ) {
		if ( ms->stateBits & SB_ISMARKED ) {
			if ( ms->lmItemSet.length() > maxItemSetLength )
				maxItemSetLength = ms->lmItemSet.length();
			ms->stateBits &= ~ SB_ISMARKED;
		}
	}

	/* The actions executed on starting to match a token. */
	graph->isolateStartState();
	graph->startState->fromStateActionTable.setAction( pd->setTokStartOrd, pd->setTokStart );
	if ( maxItemSetLength > 1 ) {
		/* The longest match action switch may be called when tokens are
		 * matched, in which case act must be initialized, there must be a
		 * case to handle the error, and the generated machine will require an
		 * error state. */
		lmSwitchHandlesError = true;
		graph->startState->toStateActionTable.setAction( pd->initActIdOrd, pd->initActId );
	}

	/* The place to store transitions to restart. It maybe possible for the
	 * restarting to affect the searching through the graph that follows. For
	 * now take the safe route and save the list of transitions to restart
	 * until after all searching is done. */
	Vector<FsmTrans*> restartTrans;

	/* Set actions that do immediate token recognition, set the longest match part
	 * id and set the token ending. */
	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
			if ( trans->lmActionTable.length() > 0 ) {
				LmActionTableEl *lmAct = trans->lmActionTable.data;
				FsmState *toState = trans->toState;
				assert( toState );

				/* Check if there are transitions out, this may be a very
				 * close approximation? Out transitions going nowhere?
				 * FIXME: Check. */
				if ( toState->outList.length() == 0 ) {
					/* Can execute the immediate action for the longest match
					 * part. Redirect the action to the start state. */
					trans->actionTable.setAction( lmAct->key, 
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
						if ( ms->stateBits & SB_ISMARKED ) {
							if ( ms->lmItemSet.length() > 0 && !ms->isFinState() )
								nonFinalNonEmptyItemSet = true;
							if ( ms->lmItemSet.length() > maxItemSetLength )
								maxItemSetLength = ms->lmItemSet.length();
							ms->stateBits &= ~ SB_ISMARKED;
						}
					}

					/* If there are reachable states that are not final and
					 * have non empty item sets or that have an item set
					 * length greater than one then we need to set tokend
					 * because the error action that matches the token will
					 * require it. */
					if ( nonFinalNonEmptyItemSet || maxItemSetLength > 1 )
						trans->actionTable.setAction( pd->setTokEndOrd, pd->setTokEnd );

					/* Some states may not know which longest match item to
					 * execute, must set it. */
					if ( maxItemSetLength > 1 ) {
						/* There are transitions out, another match may come. */
						trans->actionTable.setAction( lmAct->key, 
								lmAct->value->setActId );
					}
				}
			}
		}
	}

	/* Now that all graph searching is done it certainly safe set the
	 * restarting. It may be safe above, however this must be verified. */
	for ( Vector<FsmTrans*>::Iter rs = restartTrans; rs.lte(); rs++ )
		restart( graph, *rs );

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
			/* Need to use the select. Take note of the which items the select
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

void TokenRegion::transferScannerLeavingActions( FsmGraph *graph )
{
	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		if ( st->outActionTable.length() > 0 )
			graph->setErrorActions( st, st->outActionTable );
	}
}

FsmGraph *TokenRegion::walk( Compiler *pd )
{
	/* The longest match has it's own name scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Make each part of the longest match. */
	int numParts = 0;
	FsmGraph **parts = new FsmGraph*[tokenDefList.length()];
	for ( TokenDefListReg::Iter lmi = tokenDefList; lmi.lte(); lmi++ ) {
		/* Watch out for patternless tokens. */
		if ( lmi->join != 0 ) {
			/* Create the machine and embed the setting of the longest match id. */
			parts[numParts] = lmi->join->walk( pd );
			parts[numParts]->longMatchAction( pd->curActionOrd++, lmi );

			/* Look for tokens that accept the zero length-word. The first one found
			 * will be used as the default token. */
			if ( defaultTokenDef == 0 && parts[numParts]->startState->isFinState() )
				defaultTokenDef = lmi;

			numParts += 1;
		}
	}
	FsmGraph *retFsm = parts[0];

	if ( defaultTokenDef != 0 && defaultTokenDef->tdLangEl->ignore )
		error() << "ignore token cannot be a scanner's zero-length token" << endp;

	/* The region is empty. Return the empty set. */
	if ( numParts == 0 ) {
		retFsm = new FsmGraph();
		retFsm->lambdaFsm();
	}
	else {
		/* Before we union the patterns we need to deal with leaving actions. They
		 * are transfered to error transitions out of the final states (like local
		 * error actions) and to eof actions. In the scanner we need to forbid
		 * on_last for any final state that has an leaving action. */
		for ( int i = 0; i < numParts; i++ )
			transferScannerLeavingActions( parts[i] );

		/* Union machines one and up with machine zero. */
		FsmGraph *retFsm = parts[0];
		for ( int i = 1; i < numParts; i++ ) {
			retFsm->unionOp( parts[i] );
			afterOpMinimize( retFsm );
		}

		runLongestMatch( pd, retFsm );
		delete[] parts;
	}

	/* Pop the name scope. */
	pd->popNameScope( nameFrame );

	return retFsm;
}

/* Construct with a location and the first expression. */
Join::Join( Expression *expr )
:
	context(0),
	mark(0)
{
	exprList.append( expr );
}

/* Walk an expression node. */
FsmGraph *Join::walk( Compiler *pd )
{
	assert( exprList.length() == 1 );

	FsmGraph *retFsm = exprList.head->walk( pd );

	/* Maybe the the context. */
	if ( context != 0 ) {
		retFsm->leaveFsmAction( pd->curActionOrd++, mark );
		FsmGraph *contextGraph = context->walk( pd );
		retFsm->concatOp( contextGraph );
	}

	return retFsm;
}

void Join::makeNameTree( Compiler *pd )
{
	assert( exprList.length() == 1 );

	/* Recurse into the single expression. */
	exprList.head->makeNameTree( pd );

	/* Maybe the the context. */
	if ( context != 0 )
		context->makeNameTree( pd );
}


/* Clean up after an expression node. */
Expression::~Expression()
{
	switch ( type ) {
		case OrType: case IntersectType: case SubtractType:
		case StrongSubtractType:
			delete expression;
			delete term;
			break;
		case TermType:
			delete term;
			break;
		case BuiltinType:
			break;
	}
}

/* Evaluate a single expression node. */
FsmGraph *Expression::walk( Compiler *pd, bool lastInSeq )
{
	FsmGraph *rtnVal = 0;
	switch ( type ) {
		case OrType: {
			/* Evaluate the expression. */
			rtnVal = expression->walk( pd, false );
			/* Evaluate the term. */
			FsmGraph *rhs = term->walk( pd );
			/* Perform union. */
			rtnVal->unionOp( rhs );
			afterOpMinimize( rtnVal, lastInSeq );
			break;
		}
		case IntersectType: {
			/* Evaluate the expression. */
			rtnVal = expression->walk( pd );
			/* Evaluate the term. */
			FsmGraph *rhs = term->walk( pd );
			/* Perform intersection. */
			rtnVal->intersectOp( rhs );
			afterOpMinimize( rtnVal, lastInSeq );
			break;
		}
		case SubtractType: {
			/* Evaluate the expression. */
			rtnVal = expression->walk( pd );
			/* Evaluate the term. */
			FsmGraph *rhs = term->walk( pd );
			/* Perform subtraction. */
			rtnVal->subtractOp( rhs );
			afterOpMinimize( rtnVal, lastInSeq );
			break;
		}
		case StrongSubtractType: {
			/* Evaluate the expression. */
			rtnVal = expression->walk( pd );

			/* Evaluate the term and pad it with any* machines. */
			FsmGraph *rhs = dotStarFsm( pd );
			FsmGraph *termFsm = term->walk( pd );
			FsmGraph *trailAnyStar = dotStarFsm( pd );
			rhs->concatOp( termFsm );
			rhs->concatOp( trailAnyStar );

			/* Perform subtraction. */
			rtnVal->subtractOp( rhs );
			afterOpMinimize( rtnVal, lastInSeq );
			break;
		}
		case TermType: {
			/* Return result of the term. */
			rtnVal = term->walk( pd );
			break;
		}
		case BuiltinType: {
			/* Duplicate the builtin. */
			rtnVal = makeBuiltin( builtin, pd );
			break;
		}
	}

	return rtnVal;
}

void Expression::makeNameTree( Compiler *pd )
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

/* Clean up after a term node. */
Term::~Term()
{
	switch ( type ) {
		case ConcatType:
		case RightStartType:
		case RightFinishType:
		case LeftType:
			delete term;
			delete factorWithAug;
			break;
		case FactorWithAugType:
			delete factorWithAug;
			break;
	}
}

/* Evaluate a term node. */
FsmGraph *Term::walk( Compiler *pd, bool lastInSeq )
{
	FsmGraph *rtnVal = 0;
	switch ( type ) {
		case ConcatType: {
			/* Evaluate the Term. */
			rtnVal = term->walk( pd, false );
			/* Evaluate the FactorWithRep. */
			FsmGraph *rhs = factorWithAug->walk( pd );
			/* Perform concatenation. */
			rtnVal->concatOp( rhs );
			afterOpMinimize( rtnVal, lastInSeq );
			break;
		}
		case RightStartType: {
			/* Evaluate the Term. */
			rtnVal = term->walk( pd );

			/* Evaluate the FactorWithRep. */
			FsmGraph *rhs = factorWithAug->walk( pd );

			/* Set up the priority descriptors. The left machine gets the
			 * lower priority where as the right get the higher start priority. */
			priorDescs[0].key = pd->nextPriorKey++;
			priorDescs[0].priority = 0;
			rtnVal->allTransPrior( pd->curPriorOrd++, &priorDescs[0] );

			/* The start transitions right machine get the higher priority.
			 * Use the same unique key. */
			priorDescs[1].key = priorDescs[0].key;
			priorDescs[1].priority = 1;
			rhs->startFsmPrior( pd->curPriorOrd++, &priorDescs[1] );

			/* Perform concatenation. */
			rtnVal->concatOp( rhs );
			afterOpMinimize( rtnVal, lastInSeq );
			break;
		}
		case RightFinishType: {
			/* Evaluate the Term. */
			rtnVal = term->walk( pd );

			/* Evaluate the FactorWithRep. */
			FsmGraph *rhs = factorWithAug->walk( pd );

			/* Set up the priority descriptors. The left machine gets the
			 * lower priority where as the finishing transitions to the right
			 * get the higher priority. */
			priorDescs[0].key = pd->nextPriorKey++;
			priorDescs[0].priority = 0;
			rtnVal->allTransPrior( pd->curPriorOrd++, &priorDescs[0] );

			/* The finishing transitions of the right machine get the higher
			 * priority. Use the same unique key. */
			priorDescs[1].key = priorDescs[0].key;
			priorDescs[1].priority = 1;
			rhs->finishFsmPrior( pd->curPriorOrd++, &priorDescs[1] );

			/* Perform concatenation. */
			rtnVal->concatOp( rhs );
			afterOpMinimize( rtnVal, lastInSeq );
			break;
		}
		case LeftType: {
			/* Evaluate the Term. */
			rtnVal = term->walk( pd );

			/* Evaluate the FactorWithRep. */
			FsmGraph *rhs = factorWithAug->walk( pd );

			/* Set up the priority descriptors. The left machine gets the
			 * higher priority. */
			priorDescs[0].key = pd->nextPriorKey++;
			priorDescs[0].priority = 1;
			rtnVal->allTransPrior( pd->curPriorOrd++, &priorDescs[0] );

			/* The right machine gets the lower priority.  Since
			 * startTransPrior might unnecessarily increase the number of
			 * states during the state machine construction process (due to
			 * isolation), we use allTransPrior instead, which has the same
			 * effect. */
			priorDescs[1].key = priorDescs[0].key;
			priorDescs[1].priority = 0;
			rhs->allTransPrior( pd->curPriorOrd++, &priorDescs[1] );

			/* Perform concatenation. */
			rtnVal->concatOp( rhs );
			afterOpMinimize( rtnVal, lastInSeq );
			break;
		}
		case FactorWithAugType: {
			rtnVal = factorWithAug->walk( pd );
			break;
		}
	}
	return rtnVal;
}

void Term::makeNameTree( Compiler *pd )
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

/* Clean up after a factor with augmentation node. */
FactorWithAug::~FactorWithAug()
{
	delete factorWithRep;

	/* Walk the vector of parser actions, deleting function names. */

	/* Clean up priority descriptors. */
	if ( priorDescs != 0 )
		delete[] priorDescs;
}

void FactorWithAug::assignActions( Compiler *pd, FsmGraph *graph, int *actionOrd )
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

void FactorWithAug::assignPriorities( FsmGraph *graph, int *priorOrd )
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

void FactorWithAug::assignConditions( FsmGraph *graph )
{
	for ( int i = 0; i < conditions.length(); i++ )  {
		switch ( conditions[i].type ) {
		/* Transition actions. */
		case at_start:
			graph->startFsmCondition( conditions[i].action );
			afterOpMinimize( graph );
			break;
		case at_all:
			graph->allTransCondition( conditions[i].action );
			break;
		case at_leave:
			graph->leaveFsmCondition( conditions[i].action );
			break;
		default:
			break;
		}
	}
}


/* Evaluate a factor with augmentation node. */
FsmGraph *FactorWithAug::walk( Compiler *pd )
{
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
	FsmGraph *rtnVal = factorWithRep->walk( pd );

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

	assignConditions( rtnVal );

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

	if ( priorOrd != 0 )
		delete[] priorOrd;
	if ( actionOrd != 0 )
		delete[] actionOrd;	
	return rtnVal;
}

void FactorWithAug::makeNameTree( Compiler *pd )
{
	NameInst *prevNameInst = pd->curNameInst;
	factorWithRep->makeNameTree( pd );
	pd->curNameInst = prevNameInst;
}

/* Clean up after a factor with repetition node. */
FactorWithRep::~FactorWithRep()
{
	switch ( type ) {
		case StarType: case StarStarType: case OptionalType: case PlusType:
		case ExactType: case MaxType: case MinType: case RangeType:
			delete factorWithRep;
			break;
		case FactorWithNegType:
			delete factorWithNeg;
			break;
	}
}

/* Evaluate a factor with repetition node. */
FsmGraph *FactorWithRep::walk( Compiler *pd )
{
	FsmGraph *retFsm = 0;

	switch ( type ) {
	case StarType: {
		/* Evaluate the FactorWithRep. */
		retFsm = factorWithRep->walk( pd );
		if ( retFsm->startState->isFinState() ) {
			warning(loc) << "applying kleene star to a machine that "
					"accepts zero length word" << endl;
		}

		/* Shift over the start action orders then do the kleene star. */
		pd->curActionOrd += retFsm->shiftStartActionOrder( pd->curActionOrd );
		retFsm->starOp( );
		afterOpMinimize( retFsm );
		break;
	}
	case StarStarType: {
		/* Evaluate the FactorWithRep. */
		retFsm = factorWithRep->walk( pd );
		if ( retFsm->startState->isFinState() ) {
			warning(loc) << "applying kleene star to a machine that "
					"accepts zero length word" << endl;
		}

		/* Set up the prior descs. All gets priority one, whereas leaving gets
		 * priority zero. Make a unique key so that these priorities don't
		 * interfere with any priorities set by the user. */
		priorDescs[0].key = pd->nextPriorKey++;
		priorDescs[0].priority = 1;
		retFsm->allTransPrior( pd->curPriorOrd++, &priorDescs[0] );

		/* Leaveing gets priority 0. Use same unique key. */
		priorDescs[1].key = priorDescs[0].key;
		priorDescs[1].priority = 0;
		retFsm->leaveFsmPrior( pd->curPriorOrd++, &priorDescs[1] );

		/* Shift over the start action orders then do the kleene star. */
		pd->curActionOrd += retFsm->shiftStartActionOrder( pd->curActionOrd );
		retFsm->starOp( );
		afterOpMinimize( retFsm );
		break;
	}
	case OptionalType: {
		/* Make the null fsm. */
		FsmGraph *nu = new FsmGraph();
		nu->lambdaFsm( );

		/* Evaluate the FactorWithRep. */
		retFsm = factorWithRep->walk( pd );

		/* Perform the question operator. */
		retFsm->unionOp( nu );
		afterOpMinimize( retFsm );
		break;
	}
	case PlusType: {
		/* Evaluate the FactorWithRep. */
		retFsm = factorWithRep->walk( pd );
		if ( retFsm->startState->isFinState() ) {
			warning(loc) << "applying plus operator to a machine that "
					"accpets zero length word" << endl;
		}

		/* Need a duplicated for the star end. */
		FsmGraph *dup = new FsmGraph( *retFsm );

		/* The start func orders need to be shifted before doing the star. */
		pd->curActionOrd += dup->shiftStartActionOrder( pd->curActionOrd );

		/* Star the duplicate. */
		dup->starOp( );
		afterOpMinimize( dup );

		retFsm->concatOp( dup );
		afterOpMinimize( retFsm );
		break;
	}
	case ExactType: {
		/* Get an int from the repetition amount. */
		if ( lowerRep == 0 ) {
			/* No copies. Don't need to evaluate the factorWithRep. 
			 * This Defeats the purpose so give a warning. */
			warning(loc) << "exactly zero repetitions results "
					"in the null machine" << endl;

			retFsm = new FsmGraph();
			retFsm->lambdaFsm();
		}
		else {
			/* Evaluate the first FactorWithRep. */
			retFsm = factorWithRep->walk( pd );
			if ( retFsm->startState->isFinState() ) {
				warning(loc) << "applying repetition to a machine that "
						"accepts zero length word" << endl;
			}

			/* The start func orders need to be shifted before doing the
			 * repetition. */
			pd->curActionOrd += retFsm->shiftStartActionOrder( pd->curActionOrd );

			/* Do the repetition on the machine. Already guarded against n == 0 */
			retFsm->repeatOp( lowerRep );
			afterOpMinimize( retFsm );
		}
		break;
	}
	case MaxType: {
		/* Get an int from the repetition amount. */
		if ( upperRep == 0 ) {
			/* No copies. Don't need to evaluate the factorWithRep. 
			 * This Defeats the purpose so give a warning. */
			warning(loc) << "max zero repetitions results "
					"in the null machine" << endl;

			retFsm = new FsmGraph();
			retFsm->lambdaFsm();
		}
		else {
			/* Evaluate the first FactorWithRep. */
			retFsm = factorWithRep->walk( pd );
			if ( retFsm->startState->isFinState() ) {
				warning(loc) << "applying max repetition to a machine that "
						"accepts zero length word" << endl;
			}

			/* The start func orders need to be shifted before doing the 
			 * repetition. */
			pd->curActionOrd += retFsm->shiftStartActionOrder( pd->curActionOrd );

			/* Do the repetition on the machine. Already guarded against n == 0 */
			retFsm->optionalRepeatOp( upperRep );
			afterOpMinimize( retFsm );
		}
		break;
	}
	case MinType: {
		/* Evaluate the repeated machine. */
		retFsm = factorWithRep->walk( pd );
		if ( retFsm->startState->isFinState() ) {
			warning(loc) << "applying min repetition to a machine that "
					"accepts zero length word" << endl;
		}

		/* The start func orders need to be shifted before doing the repetition
		 * and the kleene star. */
		pd->curActionOrd += retFsm->shiftStartActionOrder( pd->curActionOrd );
	
		if ( lowerRep == 0 ) {
			/* Acts just like a star op on the machine to return. */
			retFsm->starOp( );
			afterOpMinimize( retFsm );
		}
		else {
			/* Take a duplicate for the plus. */
			FsmGraph *dup = new FsmGraph( *retFsm );

			/* Do repetition on the first half. */
			retFsm->repeatOp( lowerRep );
			afterOpMinimize( retFsm );

			/* Star the duplicate. */
			dup->starOp( );
			afterOpMinimize( dup );

			/* Tak on the kleene star. */
			retFsm->concatOp( dup );
			afterOpMinimize( retFsm );
		}
		break;
	}
	case RangeType: {
		/* Check for bogus range. */
		if ( upperRep - lowerRep < 0 ) {
			error(loc) << "invalid range repetition" << endl;

			/* Return null machine as recovery. */
			retFsm = new FsmGraph();
			retFsm->lambdaFsm();
		}
		else if ( lowerRep == 0 && upperRep == 0 ) {
			/* No copies. Don't need to evaluate the factorWithRep.  This
			 * defeats the purpose so give a warning. */
			warning(loc) << "zero to zero repetitions results "
					"in the null machine" << endl;

			retFsm = new FsmGraph();
			retFsm->lambdaFsm();
		}
		else {
			/* Now need to evaluate the repeated machine. */
			retFsm = factorWithRep->walk( pd );
			if ( retFsm->startState->isFinState() ) {
				warning(loc) << "applying range repetition to a machine that "
						"accepts zero length word" << endl;
			}

			/* The start func orders need to be shifted before doing both kinds
			 * of repetition. */
			pd->curActionOrd += retFsm->shiftStartActionOrder( pd->curActionOrd );

			if ( lowerRep == 0 ) {
				/* Just doing max repetition. Already guarded against n == 0. */
				retFsm->optionalRepeatOp( upperRep );
				afterOpMinimize( retFsm );
			}
			else if ( lowerRep == upperRep ) {
				/* Just doing exact repetition. Already guarded against n == 0. */
				retFsm->repeatOp( lowerRep );
				afterOpMinimize( retFsm );
			}
			else {
				/* This is the case that 0 < lowerRep < upperRep. Take a
				 * duplicate for the optional repeat. */
				FsmGraph *dup = new FsmGraph( *retFsm );

				/* Do repetition on the first half. */
				retFsm->repeatOp( lowerRep );
				afterOpMinimize( retFsm );

				/* Do optional repetition on the second half. */
				dup->optionalRepeatOp( upperRep - lowerRep );
				afterOpMinimize( dup );

				/* Tak on the duplicate machine. */
				retFsm->concatOp( dup );
				afterOpMinimize( retFsm );
			}
		}
		break;
	}
	case FactorWithNegType: {
		/* Evaluate the Factor. Pass it up. */
		retFsm = factorWithNeg->walk( pd );
		break;
	}}
	return retFsm;
}

void FactorWithRep::makeNameTree( Compiler *pd )
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
FsmGraph *FactorWithNeg::walk( Compiler *pd )
{
	FsmGraph *retFsm = 0;

	switch ( type ) {
	case NegateType: {
		/* Evaluate the factorWithNeg. */
		FsmGraph *toNegate = factorWithNeg->walk( pd );

		/* Negation is subtract from dot-star. */
		retFsm = dotStarFsm( pd );
		retFsm->subtractOp( toNegate );
		afterOpMinimize( retFsm );
		break;
	}
	case CharNegateType: {
		/* Evaluate the factorWithNeg. */
		FsmGraph *toNegate = factorWithNeg->walk( pd );

		/* CharNegation is subtract from dot. */
		retFsm = dotFsm( pd );
		retFsm->subtractOp( toNegate );
		afterOpMinimize( retFsm );
		break;
	}
	case FactorType: {
		/* Evaluate the Factor. Pass it up. */
		retFsm = factor->walk( pd );
		break;
	}}
	return retFsm;
}

void FactorWithNeg::makeNameTree( Compiler *pd )
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
			delete regExp;
			break;
		case ReferenceType:
			break;
		case ParenType:
			delete join;
			break;
	}
}

/* Evaluate a factor node. */
FsmGraph *Factor::walk( Compiler *pd )
{
	FsmGraph *rtnVal = 0;
	switch ( type ) {
	case LiteralType:
		rtnVal = literal->walk( pd );
		break;
	case RangeType:
		rtnVal = range->walk( pd );
		break;
	case OrExprType:
		rtnVal = reItem->walk( pd, 0 );
		break;
	case RegExprType:
		rtnVal = regExp->walk( pd, 0 );
		break;
	case ReferenceType:
		rtnVal = varDef->walk( pd );
		break;
	case ParenType:
		rtnVal = join->walk( pd );
		break;
	}

	return rtnVal;
}

void Factor::makeNameTree( Compiler *pd )
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
	}
}

/* Clean up a range object. Must delete the two literals. */
Range::~Range()
{
	delete lowerLit;
	delete upperLit;
}

bool Range::verifyRangeFsm( FsmGraph *rangeEnd )
{
	/* Must have two states. */
	if ( rangeEnd->stateList.length() != 2 )
		return false;
	/* The start state cannot be final. */
	if ( rangeEnd->startState->isFinState() )
		return false;
	/* There should be only one final state. */
	if ( rangeEnd->finStateSet.length() != 1 )
		return false;
	/* The final state cannot have any transitions out. */
	if ( rangeEnd->finStateSet[0]->outList.length() != 0 )
		return false;
	/* The start state should have only one transition out. */
	if ( rangeEnd->startState->outList.length() != 1 )
		return false;
	/* The singe transition out of the start state should not be a range. */
	FsmTrans *startTrans = rangeEnd->startState->outList.head;
	if ( startTrans->lowKey != startTrans->highKey )
		return false;
	return true;
}

/* Evaluate a range. Gets the lower an upper key and makes an fsm range. */
FsmGraph *Range::walk( Compiler *pd )
{
	/* Construct and verify the suitability of the lower end of the range. */
	FsmGraph *lowerFsm = lowerLit->walk( pd );
	if ( !verifyRangeFsm( lowerFsm ) ) {
		error(lowerLit->loc) << 
			"bad range lower end, must be a single character" << endl;
	}

	/* Construct and verify the upper end. */
	FsmGraph *upperFsm = upperLit->walk( pd );
	if ( !verifyRangeFsm( upperFsm ) ) {
		error(upperLit->loc) << 
			"bad range upper end, must be a single character" << endl;
	}

	/* Grab the keys from the machines, then delete them. */
	Key lowKey = lowerFsm->startState->outList.head->lowKey;
	Key highKey = upperFsm->startState->outList.head->lowKey;
	delete lowerFsm;
	delete upperFsm;

	/* Validate the range. */
	if ( lowKey > highKey ) {
		/* Recover by setting upper to lower; */
		error(lowerLit->loc) << "lower end of range is greater then upper end" << endl;
		highKey = lowKey;
	}

	/* Return the range now that it is validated. */
	FsmGraph *retFsm = new FsmGraph();
	retFsm->rangeFsm( lowKey, highKey );
	return retFsm;
}

/* Evaluate a literal object. */
FsmGraph *Literal::walk( Compiler *pd )
{
	/* FsmGraph to return, is the alphabet signed. */
	FsmGraph *rtnVal = 0;

	switch ( type ) {
	case Number: {
		/* Make the fsm key in int format. */
		Key fsmKey = makeFsmKeyNum( literal.data, loc, pd );
		/* Make the new machine. */
		rtnVal = new FsmGraph();
		rtnVal->concatFsm( fsmKey );
		break;
	}
	case LitString: {
		/* Make the array of keys in int format. */
		String interp;
		bool caseInsensitive;
		prepareLitString( interp, caseInsensitive, literal, loc );
		Key *arr = new Key[interp.length()];
		makeFsmKeyArray( arr, interp.data, interp.length(), pd );

		/* Make the new machine. */
		rtnVal = new FsmGraph();
		if ( caseInsensitive )
			rtnVal->concatFsmCI( arr, interp.length() );
		else
			rtnVal->concatFsm( arr, interp.length() );
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
			delete regExp;
			delete item;
			break;
		case Empty:
			break;
	}
}

/* Evaluate a regular expression object. */
FsmGraph *RegExpr::walk( Compiler *pd, RegExpr *rootRegex )
{
	/* This is the root regex, pass down a pointer to this. */
	if ( rootRegex == 0 )
		rootRegex = this;

	FsmGraph *rtnVal = 0;
	switch ( type ) {
		case RecurseItem: {
			/* Walk both items. */
			FsmGraph *fsm1 = regExp->walk( pd, rootRegex );
			FsmGraph *fsm2 = item->walk( pd, rootRegex );
			if ( fsm1 == 0 )
				rtnVal = fsm2;
			else {
				fsm1->concatOp( fsm2 );
				rtnVal = fsm1;
			}
			break;
		}
		case Empty: {
			/* FIXME: Return something here. */
			rtnVal = 0;
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
FsmGraph *ReItem::walk( Compiler *pd, RegExpr *rootRegex )
{
	/* The fsm to return, is the alphabet signed? */
	FsmGraph *rtnVal = 0;

	switch ( type ) {
		case Data: {
			/* Move the data into an integer array and make a concat fsm. */
			Key *arr = new Key[data.length()];
			makeFsmKeyArray( arr, data.data, data.length(), pd );

			/* Make the concat fsm. */
			rtnVal = new FsmGraph();
			if ( rootRegex != 0 && rootRegex->caseInsensitive )
				rtnVal->concatFsmCI( arr, data.length() );
			else
				rtnVal->concatFsm( arr, data.length() );
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
			rtnVal->minimizePartition2();
			break;
		}
		case NegOrBlock: {
			/* Get the or block and minimize it. */
			FsmGraph *fsm = orBlock->walk( pd, rootRegex );
			fsm->minimizePartition2();

			/* Make a dot fsm and subtract from it. */
			rtnVal = dotFsm( pd );
			rtnVal->subtractOp( fsm );
			rtnVal->minimizePartition2();
			break;
		}
	}

	/* If the item is followed by a star, then apply the star op. */
	if ( star ) {
		if ( rtnVal->startState->isFinState() ) {
			warning(loc) << "applying kleene star to a machine that "
					"accpets zero length word" << endl;
		}

		rtnVal->starOp();
		rtnVal->minimizePartition2();
	}
	return rtnVal;
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
FsmGraph *ReOrBlock::walk( Compiler *pd, RegExpr *rootRegex )
{
	FsmGraph *rtnVal = 0;
	switch ( type ) {
		case RecurseItem: {
			/* Evaluate the two fsm. */
			FsmGraph *fsm1 = orBlock->walk( pd, rootRegex );
			FsmGraph *fsm2 = item->walk( pd, rootRegex );
			if ( fsm1 == 0 )
				rtnVal = fsm2;
			else {
				fsm1->unionOp( fsm2 );
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
FsmGraph *ReOrItem::walk( Compiler *pd, RegExpr *rootRegex )
{
	/* The return value, is the alphabet signed? */
	FsmGraph *rtnVal = 0;
	switch ( type ) {
	case Data: {
		/* Make the or machine. */
		rtnVal = new FsmGraph();

		/* Put the or data into an array of ints. Note that we find unique
		 * keys. Duplicates are silently ignored. The alternative would be to
		 * issue warning or an error but since we can't with [a0-9a] or 'a' |
		 * 'a' don't bother here. */
		KeySet keySet;
		makeFsmUniqueKeyArray( keySet, data.data, data.length(), 
			rootRegex != 0 ? rootRegex->caseInsensitive : false, pd );

		/* Run the or operator. */
		rtnVal->orFsm( keySet.data, keySet.length() );
		break;
	}
	case Range: {
		/* Make the upper and lower keys. */
		Key lowKey = makeFsmKeyChar( lower, pd );
		Key highKey = makeFsmKeyChar( upper, pd );

		/* Validate the range. */
		if ( lowKey > highKey ) {
			/* Recover by setting upper to lower; */
			error(loc) << "lower end of range is greater then upper end" << endl;
			highKey = lowKey;
		}

		/* Make the range machine. */
		rtnVal = new FsmGraph();
		rtnVal->rangeFsm( lowKey, highKey );

		if ( rootRegex != 0 && rootRegex->caseInsensitive ) {
			if ( lowKey <= 'Z' && 'A' <= highKey ) {
				Key otherLow = lowKey < 'A' ? Key('A') : lowKey;
				Key otherHigh = 'Z' < highKey ? Key('Z') : highKey;

				otherLow = 'a' + ( otherLow - 'A' );
				otherHigh = 'a' + ( otherHigh - 'A' );

				FsmGraph *otherRange = new FsmGraph();
				otherRange->rangeFsm( otherLow, otherHigh );
				rtnVal->unionOp( otherRange );
				rtnVal->minimizePartition2();
			}
			else if ( lowKey <= 'z' && 'a' <= highKey ) {
				Key otherLow = lowKey < 'a' ? Key('a') : lowKey;
				Key otherHigh = 'z' < highKey ? Key('z') : highKey;

				otherLow = 'A' + ( otherLow - 'a' );
				otherHigh = 'A' + ( otherHigh - 'a' );

				FsmGraph *otherRange = new FsmGraph();
				otherRange->rangeFsm( otherLow, otherHigh );
				rtnVal->unionOp( otherRange );
				rtnVal->minimizePartition2();
			}
		}

		break;
	}}
	return rtnVal;
}
