/*
 * Copyright 2008-2018 Adrian Thurston <thurston@colm.net>
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


#include "ragel.h"
#include "fsmgraph.h"
#include "parsedata.h"

/* Error reporting format. */
ErrorFormat errorFormat = ErrorFormatGNU;

void FsmCtx::finalizeInstance( FsmAp *graph )
{
	/* Resolve any labels that point to multiple states. Any labels that are
	 * still around are referenced only by gotos and calls and they need to be
	 * made into deterministic entry points. */
	graph->deterministicEntry();

	/*
	 * All state construction is now complete.
	 */

	/* Transfer actions from the out action tables to eof action tables. */
	for ( StateSet::Iter state = graph->finStateSet; state.lte(); state++ )
		graph->transferOutActions( *state );

	/* Transfer global error actions. */
	for ( StateList::Iter state = graph->stateList; state.lte(); state++ )
		graph->transferErrorActions( state, 0 );
	
	if ( fsmGbl->wantDupsRemoved )
		graph->removeActionDups();

	/* Remove unreachable states. There should be no dead end states. The
	 * subtract and intersection operators are the only places where they may
	 * be created and those operators clean them up. */
	graph->removeUnreachableStates();

	/* No more fsm operations are to be done. Action ordering numbers are
	 * no longer of use and will just hinder minimization. Clear them. */
	graph->nullActionKeys();

	/* Transition priorities are no longer of use. We can clear them
	 * because they will just hinder minimization as well. Clear them. */
	graph->clearAllPriorities();

	if ( graph->ctx->minimizeOpt != MinimizeNone ) {
		/* Minimize here even if we minimized at every op. Now that function
		 * keys have been cleared we may get a more minimal fsm. */
		switch ( graph->ctx->minimizeLevel ) {
			#ifdef TO_UPGRADE_CONDS
			case MinimizeApprox:
				graph->minimizeApproximate();
				break;
			#endif
			#ifdef TO_UPGRADE_CONDS
			case MinimizeStable:
				graph->minimizeStable();
				break;
			#endif
			case MinimizePartition1:
				graph->minimizePartition1();
				break;
			case MinimizePartition2:
				graph->minimizePartition2();
				break;
		}
	}

	graph->compressTransitions();

	createNfaActions( graph );
}

void FsmCtx::analyzeAction( Action *action, InlineList *inlineList )
{
	/* FIXME: Actions used as conditions should be very constrained. */
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		if ( item->type == InlineItem::Call || item->type == InlineItem::CallExpr ||
				item->type == InlineItem::Ncall || item->type == InlineItem::NcallExpr )
		{
			action->anyCall = true;
		}

		/* Need to recurse into longest match items. */
		if ( item->type == InlineItem::LmSwitch ) {
			LongestMatch *lm = item->longestMatch;
			for ( LmPartList::Iter lmi = *lm->longestMatchList; lmi.lte(); lmi++ ) {
				if ( lmi->action != 0 )
					analyzeAction( action, lmi->action->inlineList );
			}
		}

		if ( item->type == InlineItem::LmOnLast || 
				item->type == InlineItem::LmOnNext ||
				item->type == InlineItem::LmOnLagBehind )
		{
			LongestMatchPart *lmi = item->longestMatchPart;
			if ( lmi->action != 0 )
				analyzeAction( action, lmi->action->inlineList );
		}

		if ( item->children != 0 )
			analyzeAction( action, item->children );
	}
}


/* Check actions for bad uses of fsm directives. We don't go inside longest
 * match items in actions created by ragel, since we just want the user
 * actions. */
void FsmCtx::checkInlineList( Action *act, InlineList *inlineList )
{
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		/* EOF checks. */
		if ( act->numEofRefs > 0 ) {
			switch ( item->type ) {
				/* Currently no checks. */
				default:
					break;
			}
		}

		/* Recurse. */
		if ( item->children != 0 )
			checkInlineList( act, item->children );
	}
}

void FsmCtx::checkAction( Action *action )
{
	/* Check for actions with calls that are embedded within a longest match
	 * machine. */
	if ( !action->isLmAction && action->numRefs() > 0 && action->anyCall ) {
		for ( NameInstVect::Iter ar = action->embedRoots; ar.lte(); ar++ ) {
			NameInst *check = *ar;
			while ( check != 0 ) {
				if ( check->isLongestMatch ) {
					fsmGbl->error(action->loc) << "within a scanner, fcall and fncall are permitted"
						" only in pattern actions" << endl;
					break;
				}
				check = check->parent;
			}
		}
	}

	checkInlineList( action, action->inlineList );
}

void FsmCtx::analyzeGraph( FsmAp *graph )
{
	for ( ActionList::Iter act = actionList; act.lte(); act++ )
		analyzeAction( act, act->inlineList );

	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		/* The transition list. */
		for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
			//if ( trans->condSpace != 0 ) {
			//	for ( CondSet::Iter sci = trans->condSpace->condSet; sci.lte(); sci++ )
			//		(*sci)->numCondRefs += 1;
			//}

			if ( trans->plain() ) {
				for ( ActionTable::Iter at = trans->tdap()->actionTable; at.lte(); at++ )
					at->value->numTransRefs += 1;
			}
			else {
				for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) { 
					for ( ActionTable::Iter at = cond->actionTable; at.lte(); at++ )
						at->value->numTransRefs += 1;
				}
			}
		}

		for ( ActionTable::Iter at = st->toStateActionTable; at.lte(); at++ )
			at->value->numToStateRefs += 1;

		for ( ActionTable::Iter at = st->fromStateActionTable; at.lte(); at++ )
			at->value->numFromStateRefs += 1;

		for ( ActionTable::Iter at = st->eofActionTable; at.lte(); at++ )
			at->value->numEofRefs += 1;

		//for ( OutCondSet::Iter oci = st->outCondSet; oci.lte(); oci++ )
		//	oci->action->numCondRefs += 1;

		if ( st->nfaOut != 0 ) {
			for ( NfaTransList::Iter n = *st->nfaOut; n.lte(); n++ ) {
				for ( ActionTable::Iter ati = n->pushTable; ati.lte(); ati++ )
					ati->value->numNfaRefs += 1;

				for ( ActionTable::Iter ati = n->restoreTable; ati.lte(); ati++ )
					ati->value->numNfaRefs += 1;

				for ( ActionTable::Iter ati = n->popAction; ati.lte(); ati++ )
					ati->value->numNfaRefs += 1;

				for ( ActionTable::Iter ati = n->popTest; ati.lte(); ati++ )
					ati->value->numNfaRefs += 1;
			}
		}
	}

	/* Can't count on cond references in transitions, since we don't refcount
	 * the spaces. FIXME: That would be the proper solution. */
	for ( CondSpaceMap::Iter cs = condData->condSpaceMap; cs.lte(); cs++ ) {
		for ( CondSet::Iter csi = cs->condSet; csi.lte(); csi++ )
			(*csi)->numCondRefs += 1;
	}

	/* Checks for bad usage of directives in action code. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ )
		checkAction( act );
}

/* This create an action that refs the original embed roots, if the optWrap arg
 * is supplied. */
Action *FsmCtx::newNfaWrapAction( const char *name, InlineList *inlineList, Action *optWrap )
{
	InputLoc loc;
	loc.line = 1;
	loc.col = 1;
	loc.fileName = "NONE";

	Action *action = new Action( loc, name, inlineList, nextCondId++ );

	if ( optWrap != 0 )
		action->embedRoots.append( optWrap->embedRoots );

	actionList.append( action );
	return action;
}

void FsmCtx::createNfaActions( FsmAp *fsm )
{
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		if ( st->nfaOut != 0 ) {
			for ( NfaTransList::Iter n = *st->nfaOut; n.lte(); n++ ) {
				/* Move pop restore actions into poptest. Wrap to override the
				 * condition-like testing. */
				for ( ActionTable::Iter ati = n->restoreTable; ati.lte(); ati++ ) {
					n->popTest.setAction( ati->key, ati->value );
				}

				/* Move condition evaluation into pop test. Wrap with condition
				 * execution. */
				if ( n->popCondSpace != 0 ) {
					InlineList *il1 = new InlineList;
					il1->append( new InlineItem( InputLoc(),
							n->popCondSpace, n->popCondKeys,
							InlineItem::NfaWrapConds ) );
					Action *wrap = newNfaWrapAction( "cond_wrap", il1, 0 );
					n->popTest.setAction( ORD_COND, wrap );
				}

				/* Move pop actions into pop test. Wrap to override the
				 * condition-like testing. */
				for ( ActionTable::Iter ati = n->popAction; ati.lte(); ati++ ) {

					InlineList *il1 = new InlineList;
					il1->append( new InlineItem( InputLoc(),
								ati->value, InlineItem::NfaWrapAction ) );
					Action *wrap = newNfaWrapAction( "action_wrap", il1, ati->value );
					n->popTest.setAction( ati->key, wrap );
				}
			}
		}
	}
}

void FsmCtx::prepareReduction( FsmAp *sectionGraph )
{
	/* Decide if an error state is necessary.
	 *  1. There is an error transition
	 *  2. There is a gap in the transitions
	 *  3. The longest match operator requires it. */
	if ( lmRequiresErrorState || sectionGraph->hasErrorTrans() )
		sectionGraph->errState = sectionGraph->addState();

	/* State numbers need to be assigned such that all final states have a
	 * larger state id number than all non-final states. This enables the
	 * first_final mechanism to function correctly. We also want states to be
	 * ordered in a predictable fashion. So we first apply a depth-first
	 * search, then do a stable sort by final state status, then assign
	 * numbers. */

	sectionGraph->depthFirstOrdering();
	sectionGraph->sortStatesByFinal();
	sectionGraph->setStateNumbers( 0 );
}


void translatedHostData( ostream &out, const std::string &data )
{
	const char *p = data.c_str();
	for ( const char *c = p; *c != 0; ) {
		if ( c[0] == '}' && ( c[1] == '@' || c[1] == '$' || c[1] == '=' ) ) {
			out << "@}@" << c[1];
			c += 2;
		}
		else if ( c[0] == '@' ) {
			out << "@@";
			c += 1;
		}
		// Have some escaping issues that these fix, but they lead to other problems.
		// Can be reproduced by passing "={}" through ragel and adding --colm-backend
		// else if ( c[0] == '=' ) {
		//	out << "@=";
		//	c += 1;
		//}
		// else if ( c[0] == '$' ) {
		//	out << "@$";
		//	c += 1;
		//}
		else {
			out << c[0];
			c += 1;
		}
	}
}


void FsmGbl::abortCompile( int code )
{
	throw AbortCompile( code );
}

/* Print the opening to a warning in the input, then return the error ostream. */
ostream &FsmGbl::warning( const InputLoc &loc )
{
	ostream &err = std::cerr;
	err << loc << ": warning: ";
	return err;
}

/* Print the opening to a program error, then return the error stream. */
ostream &FsmGbl::error()
{
	errorCount += 1;
	ostream &err = std::cerr;
	err << PROGNAME ": ";
	return err;
}

ostream &FsmGbl::error( const InputLoc &loc )
{
	errorCount += 1;
	ostream &err = std::cerr;
	err << loc << ": ";
	return err;
}

ostream &FsmGbl::error_plain()
{
	errorCount += 1;
	ostream &err = std::cerr;
	return err;
}


std::ostream &FsmGbl::stats()
{
	return std::cout;
}

/* Requested info. */
std::ostream &FsmGbl::info()
{
	return std::cout;
}

ostream &operator<<( ostream &out, const InputLoc &loc )
{
	assert( loc.fileName != 0 );
	switch ( errorFormat ) {
	case ErrorFormatMSVC:
		out << loc.fileName << "(" << loc.line;
		if ( loc.col )
			out << "," << loc.col;
		out << ")";
		break;

	default:
		out << loc.fileName << ":" << loc.line;
		if ( loc.col )
			out << ":" << loc.col;
		break;
	}
	return out;
}

