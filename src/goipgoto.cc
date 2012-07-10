/*
 *  Copyright 2010 Justine Tunney <jtunney@gmail.com>
 *            2001-2006 Adrian Thurston <thurston@complang.org>
 *            2004 Erich Ocean <eric.ocean@ampede.com>
 *            2005 Alan West <alan@alanz.com>
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

#include "ragel.h"
#include "cdipgoto.h"
#include "goipgoto.h"
#include "redfsm.h"
#include "gendata.h"
#include "bstmap.h"
#include <iomanip>
#include <sstream>

using std::ostringstream;
using std::streambuf;

void GoIpGotoCodeGen::writeExec()
{
	/* Must set labels immediately before writing because we may
	 * depend on the noend write option. */
	setLabelsNeeded();
	testEofUsed = false;
	outLabelUsed = false;
	out << "	{\n";
	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps := 0\n";
	if ( redFsm->anyConditions() )
		out << "	" << WIDE_ALPH_TYPE() << " _widec\n";
	if ( !noEnd ) {
		testEofUsed = true;
		out << 
			"	if " << P() << " == " << PE() <<
			" { goto _test_eof }\n";
	}
	if ( useAgainLabel() ) {
		out << 
			"	goto _resume\n"
			"\n"
			"_again:\n"
			"	switch " << vCS() << " {\n";
		AGAIN_CASES() <<
			"	default: break\n"
			"	}\n"
			"\n";
		if ( !noEnd ) {
			testEofUsed = true;
			out << 
				"	" << P() << "++\n"
				"	if " << P() << " == " << PE() <<
				" { goto _test_eof }\n";
		} else {
			out << 
				"	" << P() << "++\n";
		}
		out << "_resume:\n";
	}
	out << "	switch " << vCS() << " {\n";

	// kludge for labels before the first real case statement
	out << "	case -666: // i am a hack D:\n";
	// isFirstCase = true;

	STATE_GOTOS();
	SWITCH_DEFAULT() <<
		"	}\n";
	EXIT_STATES() << 
		"\n";
	if ( testEofUsed ) 
		out << "	_test_eof: {}\n";
	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out <<
			"	if " << P() << " == " << vEOF() << " {\n"
			"	switch " << vCS() << " {\n";
		FINISH_CASES();
		SWITCH_DEFAULT() <<
			"	}\n"
			"	}\n"
			"\n";
	}
	if ( outLabelUsed ) 
		out << "	_out: {}\n";
	out <<
		"	}\n";
}

void GoIpGotoCodeGen::STATE_IDS()
{
	if ( redFsm->startState != 0 )
		out << "var " << START() << " int = " << START_STATE_ID() << "\n";
	if ( !noFinal )
		out << "var " << FIRST_FINAL() << " int = " << FIRST_FINAL_STATE() << "\n";
	if ( !noError )
		out << "var " << ERROR() << " int = " << ERROR_STATE() << "\n";
	out << "\n";
	if ( entryPointNames.length() > 0 ) {
		for ( EntryNameVect::Iter en = entryPointNames; en.lte(); en++ ) {
			out <<
				"var " << (DATA_PREFIX() + "en_" + *en) << 
				" int = " << entryPointIds[en.pos()] << "\n";
		}
		out << "\n";
	}
}

void GoIpGotoCodeGen::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}
	ret << "{" <<
		STACK() << "[" << TOP() << "] = " << targState << "; " <<
		TOP() << "++; " <<
		CTRL_FLOW() << "goto st" << callDest << ";}";
	if ( prePushExpr != 0 )
		ret << "}";
}

void GoIpGotoCodeGen::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}
	ret << "{" <<
		STACK() << "[" << TOP() << "] = " << targState << "; " <<
		TOP() << "++; " <<
		vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";
	if ( prePushExpr != 0 )
		ret << "}";
}

void GoIpGotoCodeGen::RET( ostream &ret, bool inFinish )
{
	ret << "{" <<
		TOP() << "--; " <<
		vCS() << " = " << STACK() << "[" << TOP() << "];";
	if ( postPopExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}";
	}
	ret << CTRL_FLOW() << "goto _again;}";
}

/* Called from GotoCodeGen::STATE_GOTOS just before writing the gotos
 * for each state. */
void GoIpGotoCodeGen::GOTO_HEADER( RedStateAp *state )
{
	bool anyWritten = IN_TRANS_ACTIONS( state );
	if ( state->labelNeeded ) 
		out << "st" << state->id << ":\n";
	if ( state->toStateAction != 0 ) {
		/* Remember that we wrote an action. Write every action in the list. */
		anyWritten = true;
		for ( GenActionTable::Iter item = state->toStateAction->key; item.lte(); item++ ) {
			ACTION( out, item->value, state->id, false, 
				state->toStateAction->anyNextStmt() );
		}
	}
	/* Advance and test buffer pos. */
	if ( state->labelNeeded ) {
		if ( !noEnd ) {
			out <<
				"	" << P() << "++\n"
				"	if " << P() << " == " << PE() <<
				" { goto _test_eof" << state->id << " }\n";
		}
		else {
			out << 
				"	" << P() << "++\n";
		}
	}

	// emulate c switch statement behavior
	out << "	fallthrough\n";
	// if (!isFirstCase) {
	// 	out << "	fallthrough\n";
	// } else {
	// 	isFirstCase = false;
	// }

	/* Give the state a switch case. */
	out << "case " << state->id << ":\n";
	if ( state->fromStateAction != 0 ) {
		/* Remember that we wrote an action. Write every action in the list. */
		anyWritten = true;
		for ( GenActionTable::Iter item = state->fromStateAction->key; item.lte(); item++ ) {
			ACTION( out, item->value, state->id, false,
				state->fromStateAction->anyNextStmt() );
		}
	}
	if ( anyWritten )
		genLineDirective( out );
	/* Record the prev state if necessary. */
	if ( state->anyRegCurStateRef() )
		out << "	_ps = " << state->id << "\n";
}

ostream &GoIpGotoCodeGen::STATIC_VAR( string type, string name )
{
	out << "var " << name << " " << type;
	return out;
}

void GoIpGotoCodeGen::LM_SWITCH( ostream &ret, GenInlineItem *item, 
				 int targState, int inFinish, bool csForced )
{
	ret << "	switch " << ACT() << " {\n";
	bool haveDefault = false;
	for ( GenInlineList::Iter lma = *item->children; lma.lte(); lma++ ) {
		/* Write the case label, the action and the case break. */
		if ( lma->lmId < 0 ) {
			ret << "	default:\n";
			haveDefault = true;
		}
		else
			ret << "	case " << lma->lmId << ":\n";
		/* Write the block and close it off. */
		ret << "	{";
		INLINE_LIST( ret, lma->children, targState, inFinish, csForced );
		ret << "}\n";
		ret << "	break\n";
	}
	ret << "	}\n	";
}

string GoIpGotoCodeGen::GET_KEY()
{
	ostringstream ret;
	if ( getKeyExpr != 0 ) { 
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, getKeyExpr, 0, false, false );
		ret << ")";
	}
	else {
		/* Expression for retrieving the key, use simple dereference. */
		ret << "data[" << P() << "]";
	}
	return ret.str();
}

void GoIpGotoCodeGen::emitSingleSwitch( RedStateAp *state )
{
	/* Load up the singles. */
	int numSingles = state->outSingle.length();
	RedTransEl *data = state->outSingle.data;
	if ( numSingles == 1 ) {
		/* If there is a single single key then write it out as an if. */
		out << "\tif " << GET_WIDE_KEY(state) << " == " << 
			KEY(data[0].lowKey) << " { "; 
		/* Virtual function for writing the target of the transition. */
		TRANS_GOTO(data[0].value, 0) << " }\n";
	}
	else if ( numSingles > 1 ) {
		/* Write out single keys in a switch if there is more than one. */
		out << "\tswitch " << GET_WIDE_KEY(state) << " {\n";
		/* Write out the single indicies. */
		for ( int j = 0; j < numSingles; j++ ) {
			out << "\t\tcase " << KEY(data[j].lowKey) << ": ";
			TRANS_GOTO(data[j].value, 0) << "\n";
		}
		/* Emits a default case for D code. */
		SWITCH_DEFAULT();
		/* Close off the transition switch. */
		out << "\t}\n";
	}
}

void GoIpGotoCodeGen::emitRangeBSearch( RedStateAp *state, int level, int low, int high )
{
	/* Get the mid position, staying on the lower end of the range. */
	int mid = (low + high) >> 1;
	RedTransEl *data = state->outRange.data;
	/* Determine if we need to look higher or lower. */
	bool anyLower = mid > low;
	bool anyHigher = mid < high;
	/* Determine if the keys at mid are the limits of the alphabet. */
	bool limitLow = data[mid].lowKey == keyOps->minKey;
	bool limitHigh = data[mid].highKey == keyOps->maxKey;
	if ( anyLower && anyHigher ) {
		/* Can go lower and higher than mid. */
		out << TABS(level) << "if " << GET_WIDE_KEY(state) << " < " << 
			KEY(data[mid].lowKey) << " {\n";
		emitRangeBSearch( state, level+1, low, mid-1 );
		out << TABS(level) << "} else if " << GET_WIDE_KEY(state) << " > " << 
			KEY(data[mid].highKey) << " {\n";
		emitRangeBSearch( state, level+1, mid+1, high );
		out << TABS(level) << "} else {\n";
		TRANS_GOTO(data[mid].value, level+1) << "\n";
		out << TABS(level) << "}\n";
	}
	else if ( anyLower && !anyHigher ) {
		/* Can go lower than mid but not higher. */
		out << TABS(level) << "if " << GET_WIDE_KEY(state) << " < " << 
			KEY(data[mid].lowKey) << " {\n";
		emitRangeBSearch( state, level+1, low, mid-1 );
		/* if the higher is the highest in the alphabet then there is no
		 * sense testing it. */
		if ( limitHigh ) {
			out << TABS(level) << "} else {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if " << GET_WIDE_KEY(state) << " <= " << 
				KEY(data[mid].highKey) << " {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
	}
	else if ( !anyLower && anyHigher ) {
		/* Can go higher than mid but not lower. */
		out << TABS(level) << "if " << GET_WIDE_KEY(state) << " > " << 
			KEY(data[mid].highKey) << " {\n";
		emitRangeBSearch( state, level+1, mid+1, high );
		/* If the lower end is the lowest in the alphabet then there is no
		 * sense testing it. */
		if ( limitLow ) {
			out << TABS(level) << "} else {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if " << GET_WIDE_KEY(state) << " >= " << 
				KEY(data[mid].lowKey) << " {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
	}
	else {
		/* Cannot go higher or lower than mid. It's mid or bust. What
		 * tests to do depends on limits of alphabet. */
		if ( !limitLow && !limitHigh ) {
			out << TABS(level) << "if " << KEY(data[mid].lowKey) << " <= " << 
				GET_WIDE_KEY(state) << " && " << GET_WIDE_KEY(state) << " <= " << 
				KEY(data[mid].highKey) << " { ";
			TRANS_GOTO(data[mid].value, 0) << " }\n";
		}
		else if ( limitLow && !limitHigh ) {
			out << TABS(level) << "if " << GET_WIDE_KEY(state) << " <= " << 
				KEY(data[mid].highKey) << " { ";
			TRANS_GOTO(data[mid].value, 0) << " }\n";
		}
		else if ( !limitLow && limitHigh ) {
			out << TABS(level) << "if " << KEY(data[mid].lowKey) << " <= " << 
				GET_WIDE_KEY(state) << " { ";
			TRANS_GOTO(data[mid].value, 0) << " }\n";
		}
		else {
			/* Both high and low are at the limit. No tests to do. */
			TRANS_GOTO(data[mid].value, level+1) << "\n";
		}
	}
}

void GoIpGotoCodeGen::COND_TRANSLATE( GenStateCond *stateCond, int level )
{
	GenCondSpace *condSpace = stateCond->condSpace;
	out << TABS(level) << "_widec = " << CAST(WIDE_ALPH_TYPE()) << "(" <<
		KEY(condSpace->baseKey) << " + (" << GET_KEY() << 
		" - " << KEY(keyOps->minKey) << "))\n";
	for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
		out << TABS(level) << "if ";
		CONDITION( out, *csi );
		Size condValOffset = ((1 << csi.pos()) * keyOps->alphSize());
		out << " { _widec += " << condValOffset << " }\n";
	}
}

void GoIpGotoCodeGen::STATE_GOTO_ERROR()
{
	/* In the error state we need to emit some stuff that usually goes into
	 * the header. */
	RedStateAp *state = redFsm->errState;
	bool anyWritten = IN_TRANS_ACTIONS( state );

	/* No case label needed since we don't switch on the error state. */
	if ( anyWritten )
		genLineDirective( out );

	if ( state->labelNeeded ) 
		out << "st" << state->id << ":\n";

	/* Break out here. */
	outLabelUsed = true;
	out << vCS() << " = " << state->id << ";\n";
	out << "	goto _out;\n";
}


ostream &GoIpGotoCodeGen::STATE_GOTOS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st == redFsm->errState )
			STATE_GOTO_ERROR();
		else {
			/* Writing code above state gotos. */
			GOTO_HEADER( st );
			if ( st->stateCondVect.length() > 0 ) {
				out << "	_widec = " << GET_KEY() << "\n";
				emitCondBSearch( st, 1, 0, st->stateCondVect.length() - 1 );
			}
			/* Try singles. */
			if ( st->outSingle.length() > 0 )
				emitSingleSwitch( st );
			/* Default case is to binary search for the ranges, if that fails then */
			if ( st->outRange.length() > 0 )
				emitRangeBSearch( st, 1, 0, st->outRange.length() - 1 );
			/* Write the default transition. */
			TRANS_GOTO( st->defTrans, 1 ) << "\n";
		}
	}
	return out;
}

/* Emit the goto to take for a given transition. */
ostream &GoIpGotoCodeGen::TRANS_GOTO( RedTransAp *trans, int level )
{
	if ( trans->action != 0 ) {
		/* Go to the transition which will go to the state. */
		out << TABS(level) << "goto tr" << trans->id;
	} else {
		/* Go directly to the target state. */
		out << TABS(level) << "goto st" << trans->targ->id;
	}
	return out;
}

void GoIpGotoCodeGen::writeInit()
{
	if ( !noCS )
		out << "\t" << vCS() << " = " << START() << "\n";
	/* If there are any calls, then the stack top needs initialization. */
	if ( redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "\t" << TOP() << " = 0\n";
	if ( hasLongestMatch ) {
		out << 
			"	" << TOKSTART() << " = " << NULL_ITEM() << "\n"
			"	" << TOKEND() << " = " << NULL_ITEM() << "\n"
			"	" << ACT() << " = 0\n";
	}
}

void GoIpGotoCodeGen::writeData()
{
	STATE_IDS();
}

void gothicLineDirective( ostream &out, const char *fileName, int line )
{
	/* Write the preprocessor line info for to the input file. */
	out << "// line " << line  << " \"";
	for ( const char *pc = fileName; *pc != 0; pc++ ) {
		if ( *pc == '\\' )
			out << "\\\\";
		else
			out << *pc;
	}
	out << "\"\n";
}

void GoIpGotoCodeGen::genLineDirective( ostream &out )
{
	streambuf *sbuf = out.rdbuf();
	output_filter *filter = static_cast<output_filter*>(sbuf);
	gothicLineDirective( out, filter->fileName, filter->line + 1 );
}

bool GoIpGotoCodeGen::IN_TRANS_ACTIONS( RedStateAp *state )
{
	bool anyWritten = false;
	/* Emit any transitions that have actions and that go to this state. */
	for ( int it = 0; it < state->numInTrans; it++ ) {
		RedTransAp *trans = state->inTrans[it];
		if ( trans->action != 0 && trans->labelNeeded ) {
			/* Remember that we wrote an action so we know to write the
			 * line directive for going back to the output. */
			anyWritten = true;
			/* Write the label for the transition so it can be jumped to. */
			out << "tr" << trans->id << ":\n";
			/* If the action contains a next, then we must preload the current
			 * state since the action may or may not set it. */
			if ( trans->action->anyNextStmt() )
				out << "	" << vCS() << " = " << trans->targ->id << "\n";
			/* Write each action in the list. */
			for ( GenActionTable::Iter item = trans->action->key; item.lte(); item++ ) {
				ACTION( out, item->value, trans->targ->id, false, 
					trans->action->anyNextStmt() );
			}
			/* If the action contains a next then we need to reload, otherwise
			 * jump directly to the target state. */
			if ( trans->action->anyNextStmt() )
				out << "\tgoto _again\n";
			else
				out << "\tgoto st" << trans->targ->id << "\n";
		}
	}
	return anyWritten;
}

void GoIpGotoCodeGen::ACTION( ostream &ret, GenAction *action, int targState, 
			      bool inFinish, bool csForced )
{
	/* Write the preprocessor line info for going into the source file. */
	gothicLineDirective( ret, action->loc.fileName, action->loc.line );
	/* Write the block and close it off. */
	ret << "\t{";
	INLINE_LIST( ret, action->inlineList, targState, inFinish, csForced );
	ret << "}\n";
}

void GoIpGotoCodeGen::CONDITION( ostream &ret, GenAction *condition )
{
	ret << "\n";
	gothicLineDirective( ret, condition->loc.fileName, condition->loc.line );
	INLINE_LIST( ret, condition->inlineList, 0, false, false );
}

ostream &GoIpGotoCodeGen::FINISH_CASES()
{
	bool anyWritten = false;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofAction != 0 ) {
			if ( st->eofAction->eofRefs == 0 )
				st->eofAction->eofRefs = new IntSet;
			st->eofAction->eofRefs->insert( st->id );
		}
	}
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			out << "	case " << st->id << ": goto tr" << st->eofTrans->id << "\n";
		}
	}
	for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		if ( act->eofRefs != 0 ) {
			/* multiple values in a case statement */
			out << "	case ";
			bool first = true;
			for ( IntSet::Iter pst = *act->eofRefs; pst.lte(); pst++ ) {
				if (first) {
					first = false;
				} else {
					out << ", ";
				}
				out << *pst;
			}
			out << ":\n";
			/* Remember that we wrote a trans so we know to write the
			 * line directive for going back to the output. */
			anyWritten = true;
			/* Write each action in the eof action list. */
			for ( GenActionTable::Iter item = act->key; item.lte(); item++ )
				ACTION( out, item->value, STATE_ERR_STATE, true, false );
			out << "\tbreak\n";
		}
	}
	if ( anyWritten )
		genLineDirective( out );
	return out;
}
