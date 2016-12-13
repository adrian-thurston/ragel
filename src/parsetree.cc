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

#include <assert.h>
#include <stdbool.h>

#include <iostream>

#include "fsmgraph.h"
#include "compiler.h"
#include "parsetree.h"

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
	char *end = 0;
	bool backtick = srcString[0] == '`';

	if ( !backtick ) {
		end = srcString.data + srcString.length() - 1;

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
	}
	else {
		end = srcString.data + srcString.length();
	}

	char *dest = result.data;
	int len = 0;
	while ( src != end ) {
		if ( !backtick && *src == '\\' ) {
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
	switch ( ut1.typeId ) {
		case TYPE_TREE:
		case TYPE_REF:
			if ( ut1.langEl < ut2.langEl )
				return -1;
			else if ( ut1.langEl > ut2.langEl )
				return 1;
			break;
		case TYPE_ITER:
			if ( ut1.iterDef < ut2.iterDef )
				return -1;
			else if ( ut1.iterDef > ut2.iterDef )
				return 1;
			break;

		case TYPE_NOTYPE:
		case TYPE_NIL:
		case TYPE_INT:
		case TYPE_BOOL:
		case TYPE_LIST_PTRS:
		case TYPE_MAP_PTRS:
			break;

		case TYPE_STRUCT:
			if ( ut1.structEl < ut2.structEl )
				return -1;
			else if ( ut1.structEl > ut2.structEl )
				return 1;
			break;
		case TYPE_GENERIC:
			if ( ut1.generic < ut2.generic )
				return -1;
			else if ( ut1.generic > ut2.generic )
				return 1;
			break;
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

int CmpUniqueGeneric::compare( const UniqueGeneric &ut1, const UniqueGeneric &ut2 )
{
	if ( ut1.type < ut2.type )
		return -1;
	else if ( ut1.type > ut2.type )
		return 1;
	else if ( ut1.value < ut2.value )
		return -1;
	else if ( ut1.value > ut2.value )
		return 1;
	else {
		switch ( ut1.type ) {
		case UniqueGeneric::List:
		case UniqueGeneric::ListEl:
		case UniqueGeneric::Parser:
			break;

		case UniqueGeneric::Map:
		case UniqueGeneric::MapEl:
			if ( ut1.key < ut2.key )
				return -1;
			else if ( ut1.key > ut2.key )
				return 1;
			break;
		}
	}
	return 0;
}

FsmGraph *LexDefinition::walk( Compiler *pd )
{
	/* Recurse on the expression. */
	FsmGraph *rtnVal = join->walk( pd );

	/* If the expression below is a join operation with multiple expressions
	 * then it just had epsilon transisions resolved. If it is a join
	 * with only a single expression then run the epsilon op now. */
	if ( join->expr != 0 )
		rtnVal->epsilonOp();

	return rtnVal;
}

void RegionImpl::makeNameTree( const InputLoc &loc, Compiler *pd )
{
	NameInst *nameInst = new NameInst( pd->nextNameId++ );
	pd->nameInstList.append( nameInst );

	/* Guess we do this now. */
	makeActions( pd );

	/* Save off the name inst into the token region. This is only legal for
	 * token regions because they are only ever referenced once (near the root
	 * of the name tree). They cannot have more than one corresponding name
	 * inst. */
	assert( regionNameInst == 0 );
	regionNameInst = nameInst;
}

InputLoc TokenInstance::getLoc()
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

Action *RegionImpl::newAction( Compiler *pd, const InputLoc &loc, 
		const String &name, InlineList *inlineList )
{
	Action *action = Action::cons( loc, name, inlineList );
	pd->actionList.append( action );
	action->isLmAction = true;
	return action;
}

void RegionImpl::makeActions( Compiler *pd )
{
	/* Make actions that set the action id. */
	for ( TokenInstanceListReg::Iter lmi = tokenInstanceList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = InlineList::cons();
		inlineList->append( InlineItem::cons( lmi->getLoc(), this, lmi, 
				InlineItem::LmSetActId ) );
		char *actName = new char[50];
		sprintf( actName, "store%i", lmi->longestMatchId );
		lmi->setActId = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart on the last character. */
	for ( TokenInstanceListReg::Iter lmi = tokenInstanceList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = InlineList::cons();
		inlineList->append( InlineItem::cons( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnLast ) );
		char *actName = new char[50];
		sprintf( actName, "imm%i", lmi->longestMatchId );
		lmi->actOnLast = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart on the next
	 * character.  These actions will set tokend themselves (it is the current
	 * char). */
	for ( TokenInstanceListReg::Iter lmi = tokenInstanceList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = InlineList::cons();
		inlineList->append( InlineItem::cons( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnNext ) );
		char *actName = new char[50];
		sprintf( actName, "lagh%i", lmi->longestMatchId );
		lmi->actOnNext = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart at tokend. These
	 * actions execute some time after matching the last char. */
	for ( TokenInstanceListReg::Iter lmi = tokenInstanceList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = InlineList::cons();
		inlineList->append( InlineItem::cons( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnLagBehind ) );
		char *actName = new char[50];
		sprintf( actName, "lag%i", lmi->longestMatchId );
		lmi->actLagBehind = newAction( pd, lmi->getLoc(), actName, inlineList );
	}

	InputLoc loc;
	loc.line = 1;
	loc.col = 1;

	/* Create the error action. */
	InlineList *il6 = InlineList::cons();
	il6->append( InlineItem::cons( loc, this, 0, InlineItem::LmSwitch ) );
	lmActSelect = newAction( pd, loc, "lagsel", il6 );
}

void RegionImpl::restart( FsmGraph *graph, FsmTrans *trans )
{
	FsmState *fromState = trans->fromState;
	graph->detachTrans( fromState, trans->toState, trans );
	graph->attachTrans( fromState, graph->startState, trans );
}

void RegionImpl::runLongestMatch( Compiler *pd, FsmGraph *graph )
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

void RegionImpl::transferScannerLeavingActions( FsmGraph *graph )
{
	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		if ( st->outActionTable.length() > 0 )
			graph->setErrorActions( st, st->outActionTable );
	}
}

FsmGraph *RegionImpl::walk( Compiler *pd )
{
	/* Make each part of the longest match. */
	int numParts = 0;
	FsmGraph **parts = new FsmGraph*[tokenInstanceList.length()];
	for ( TokenInstanceListReg::Iter lmi = tokenInstanceList; lmi.lte(); lmi++ ) {
		/* Watch out for patternless tokens. */
		if ( lmi->join != 0 ) {
			/* Create the machine and embed the setting of the longest match id. */
			parts[numParts] = lmi->join->walk( pd );
			parts[numParts]->longMatchAction( pd->curActionOrd++, lmi );

			/* Look for tokens that accept the zero length-word. The first one found
			 * will be used as the default token. */
			if ( defaultTokenInstance == 0 && parts[numParts]->startState->isFinState() )
				defaultTokenInstance = lmi;

			numParts += 1;
		}
	}
	FsmGraph *retFsm = parts[0];

	if ( defaultTokenInstance != 0 && defaultTokenInstance->tokenDef->tdLangEl->isIgnore )
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

	/* Need the entry point for the region. */
	retFsm->setEntry( regionNameInst->id, retFsm->startState );

	return retFsm;
}

/* Walk an expression node. */
FsmGraph *LexJoin::walk( Compiler *pd )
{
	FsmGraph *retFsm = expr->walk( pd );

	/* Maybe the the context. */
	if ( context != 0 ) {
		retFsm->leaveFsmAction( pd->curActionOrd++, mark );
		FsmGraph *contextGraph = context->walk( pd );
		retFsm->concatOp( contextGraph );
	}

	return retFsm;
}

/* Clean up after an expression node. */
LexExpression::~LexExpression()
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
FsmGraph *LexExpression::walk( Compiler *pd, bool lastInSeq )
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

/* Clean up after a term node. */
LexTerm::~LexTerm()
{
	switch ( type ) {
		case ConcatType:
		case RightStartType:
		case RightFinishType:
		case LeftType:
			delete term;
			delete factorAug;
			break;
		case FactorAugType:
			delete factorAug;
			break;
	}
}

/* Evaluate a term node. */
FsmGraph *LexTerm::walk( Compiler *pd, bool lastInSeq )
{
	FsmGraph *rtnVal = 0;
	switch ( type ) {
		case ConcatType: {
			/* Evaluate the Term. */
			rtnVal = term->walk( pd, false );
			/* Evaluate the LexFactorRep. */
			FsmGraph *rhs = factorAug->walk( pd );
			/* Perform concatenation. */
			rtnVal->concatOp( rhs );
			afterOpMinimize( rtnVal, lastInSeq );
			break;
		}
		case RightStartType: {
			/* Evaluate the Term. */
			rtnVal = term->walk( pd );

			/* Evaluate the LexFactorRep. */
			FsmGraph *rhs = factorAug->walk( pd );

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

			/* Evaluate the LexFactorRep. */
			FsmGraph *rhs = factorAug->walk( pd );

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

			/* Evaluate the LexFactorRep. */
			FsmGraph *rhs = factorAug->walk( pd );

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
		case FactorAugType: {
			rtnVal = factorAug->walk( pd );
			break;
		}
	}
	return rtnVal;
}

LexFactorAug::~LexFactorAug()
{
	delete factorRep;
}

void LexFactorAug::assignActions( Compiler *pd, FsmGraph *graph, int *actionOrd )
{
	/* Assign actions. */
	for ( int i = 0; i < actions.length(); i++ )  {
		switch ( actions[i].type ) {
		case at_start:
			graph->startFsmAction( actionOrd[i], actions[i].action );
			afterOpMinimize( graph );
			break;
		case at_leave:
			graph->leaveFsmAction( actionOrd[i], actions[i].action );
			break;
		}
	}
}

/* Evaluate a factor with augmentation node. */
FsmGraph *LexFactorAug::walk( Compiler *pd )
{
	/* Make the array of function orderings. */
	int *actionOrd = 0;
	if ( actions.length() > 0 )
		actionOrd = new int[actions.length()];
	
	/* First walk the list of actions, assigning order to all starting
	 * actions. */
	for ( int i = 0; i < actions.length(); i++ ) {
		if ( actions[i].type == at_start )
			actionOrd[i] = pd->curActionOrd++;
	}

	/* Evaluate the factor with repetition. */
	FsmGraph *rtnVal = factorRep->walk( pd );

	/* Compute the remaining action orderings. */
	for ( int i = 0; i < actions.length(); i++ ) {
		if ( actions[i].type != at_start )
			actionOrd[i] = pd->curActionOrd++;
	}

	assignActions( pd, rtnVal , actionOrd );

	if ( actionOrd != 0 )
		delete[] actionOrd;	
	return rtnVal;
}


/* Clean up after a factor with repetition node. */
LexFactorRep::~LexFactorRep()
{
	switch ( type ) {
		case StarType: case StarStarType: case OptionalType: case PlusType:
		case ExactType: case MaxType: case MinType: case RangeType:
			delete factorRep;
			break;
		case FactorNegType:
			delete factorNeg;
			break;
	}
}

/* Evaluate a factor with repetition node. */
FsmGraph *LexFactorRep::walk( Compiler *pd )
{
	FsmGraph *retFsm = 0;

	switch ( type ) {
	case StarType: {
		/* Evaluate the LexFactorRep. */
		retFsm = factorRep->walk( pd );
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
		/* Evaluate the LexFactorRep. */
		retFsm = factorRep->walk( pd );
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

		/* Evaluate the LexFactorRep. */
		retFsm = factorRep->walk( pd );

		/* Perform the question operator. */
		retFsm->unionOp( nu );
		afterOpMinimize( retFsm );
		break;
	}
	case PlusType: {
		/* Evaluate the LexFactorRep. */
		retFsm = factorRep->walk( pd );
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
			/* No copies. Don't need to evaluate the factorRep. 
			 * This Defeats the purpose so give a warning. */
			warning(loc) << "exactly zero repetitions results "
					"in the null machine" << endl;

			retFsm = new FsmGraph();
			retFsm->lambdaFsm();
		}
		else {
			/* Evaluate the first LexFactorRep. */
			retFsm = factorRep->walk( pd );
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
			/* No copies. Don't need to evaluate the factorRep. 
			 * This Defeats the purpose so give a warning. */
			warning(loc) << "max zero repetitions results "
					"in the null machine" << endl;

			retFsm = new FsmGraph();
			retFsm->lambdaFsm();
		}
		else {
			/* Evaluate the first LexFactorRep. */
			retFsm = factorRep->walk( pd );
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
		retFsm = factorRep->walk( pd );
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
			/* No copies. Don't need to evaluate the factorRep.  This
			 * defeats the purpose so give a warning. */
			warning(loc) << "zero to zero repetitions results "
					"in the null machine" << endl;

			retFsm = new FsmGraph();
			retFsm->lambdaFsm();
		}
		else {
			/* Now need to evaluate the repeated machine. */
			retFsm = factorRep->walk( pd );
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
	case FactorNegType: {
		/* Evaluate the Factor. Pass it up. */
		retFsm = factorNeg->walk( pd );
		break;
	}}
	return retFsm;
}


/* Clean up after a factor with negation node. */
LexFactorNeg::~LexFactorNeg()
{
	switch ( type ) {
		case NegateType:
		case CharNegateType:
			delete factorNeg;
			break;
		case FactorType:
			delete factor;
			break;
	}
}

/* Evaluate a factor with negation node. */
FsmGraph *LexFactorNeg::walk( Compiler *pd )
{
	FsmGraph *retFsm = 0;

	switch ( type ) {
	case NegateType: {
		/* Evaluate the factorNeg. */
		FsmGraph *toNegate = factorNeg->walk( pd );

		/* Negation is subtract from dot-star. */
		retFsm = dotStarFsm( pd );
		retFsm->subtractOp( toNegate );
		afterOpMinimize( retFsm );
		break;
	}
	case CharNegateType: {
		/* Evaluate the factorNeg. */
		FsmGraph *toNegate = factorNeg->walk( pd );

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

/* Clean up after a factor node. */
LexFactor::~LexFactor()
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
FsmGraph *LexFactor::walk( Compiler *pd )
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
			if ( rtnVal == 0 ) {
				rtnVal = new FsmGraph();
				rtnVal->lambdaFsm();
			}
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
