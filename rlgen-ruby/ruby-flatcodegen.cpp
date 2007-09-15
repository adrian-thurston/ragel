/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@cs.queensu.ca>
 *  Copyright 2007 Victor Hugo Borja <vic@rubyforge.org>
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

#include "ruby-flatcodegen.h"
#include "rlgen-ruby.h"
#include "redfsm.h"
#include "gendata.h"

using std::ostream;
using std::string;

std::ostream &RubyFlatCodeGen::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\twhen " << act->actionId << "\n";
			ACTION( out, act, 0, false );
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &RubyFlatCodeGen::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\twhen " << act->actionId << "\n";
			ACTION( out, act, 0, false );
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &RubyFlatCodeGen::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\twhen " << act->actionId << "\n";
			ACTION( out, act, 0, true );
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &RubyFlatCodeGen::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\twhen " << act->actionId << "\n";
			ACTION( out, act, 0, false );
		}
	}

	genLineDirective( out );
	return out;
}


std::ostream &RubyFlatCodeGen::KEYS()
{
	START_ARRAY_LINE();
	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Emit just low key and high key. */
		ARRAY_ITEM( KEY( st->lowKey ), ++totalTrans, false );
		ARRAY_ITEM( KEY( st->highKey ), ++totalTrans, false );
		if ( ++totalTrans % IALL == 0 )
			out << "\n\t";

	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT( 0 ), ++totalTrans, true );
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyFlatCodeGen::INDICIES()
{
	int totalTrans = 0;
	START_ARRAY_LINE();
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->transList != 0 ) {
			/* Walk the singles. */
			unsigned long long span = keyOps->span( st->lowKey, st->highKey );
			for ( unsigned long long pos = 0; pos < span; pos++ ) {
				ARRAY_ITEM( KEY( st->transList[pos]->id ), ++totalTrans, false );
			}
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 )
			ARRAY_ITEM( KEY( st->defTrans->id ), ++totalTrans, false );
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT( 0 ), ++totalTrans, true );
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyFlatCodeGen::FLAT_INDEX_OFFSET()
{
	START_ARRAY_LINE();
	int totalStateNum = 0, curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		ARRAY_ITEM( INT( curIndOffset ), ++totalStateNum, st.last() );
		/* Move the index offset ahead. */
		if ( st->transList != 0 )
			curIndOffset += keyOps->span( st->lowKey, st->highKey );

		if ( st->defTrans != 0 )
			curIndOffset += 1;
	}

	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyFlatCodeGen::KEY_SPANS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		unsigned long long span = 0;
		if ( st->transList != 0 )
			span = keyOps->span( st->lowKey, st->highKey );
		ARRAY_ITEM( INT( span ), ++totalStateNum, st.last() );
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyFlatCodeGen::TO_STATE_ACTIONS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		ARRAY_ITEM( INT( TO_STATE_ACTION(st) ), ++totalStateNum, st.last() );
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyFlatCodeGen::FROM_STATE_ACTIONS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		ARRAY_ITEM( INT( FROM_STATE_ACTION(st) ), ++totalStateNum, st.last() );
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyFlatCodeGen::EOF_ACTIONS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		ARRAY_ITEM( INT( EOF_ACTION(st) ), ++totalStateNum, st.last() );
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyFlatCodeGen::TRANS_TARGS()
{
	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	START_ARRAY_LINE();

	int totalStates = 0;
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Write out the target state. */
		RedTransAp *trans = transPtrs[t];
		ARRAY_ITEM( INT( trans->targ->id ), ++totalStates, t >= redFsm->transSet.length()-1  );
	}
	END_ARRAY_LINE();
	delete[] transPtrs;
	return out;
}


std::ostream &RubyFlatCodeGen::TRANS_ACTIONS()
{
	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	START_ARRAY_LINE();
	int totalAct = 0;
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Write the function for the transition. */
		RedTransAp *trans = transPtrs[t];
		ARRAY_ITEM( INT( TRANS_ACTION( trans ) ), ++totalAct, t >= redFsm->transSet.length()-1 );
	}
	END_ARRAY_LINE();
	delete[] transPtrs;
	return out;
}


void RubyFlatCodeGen::LOCATE_TRANS()
{
	out <<
		"	_keys = " << CS() << " << 1\n"
		"	_inds = " << IO() << "[" << CS() << "]\n"
		"	_slen = " << SP() << "[" << CS() << "]\n"
		"	_trans = if (   _slen > 0 && \n"
		"			" << K() << "[_keys] <= " << GET_WIDE_KEY() << " && \n"
		"			" << GET_WIDE_KEY() << " <= " << K() << "[_keys + 1] \n"
		"		    ) then\n"
		"			" << I() << "[ _inds + " << GET_WIDE_KEY() << " - " << K() << "[_keys] ] \n"
		"		 else \n"
		"			" << I() << "[ _inds + _slen ]\n"
		"		 end\n"
		"";
	
}

std::ostream &RubyFlatCodeGen::COND_INDEX_OFFSET()
{
	START_ARRAY_LINE();
	int totalStateNum = 0, curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		ARRAY_ITEM( INT( curIndOffset ), ++totalStateNum, st.last() );
		/* Move the index offset ahead. */
		if ( st->condList != 0 )
			curIndOffset += keyOps->span( st->condLowKey, st->condHighKey );
	}
	END_ARRAY_LINE();
	return out;
}

void RubyFlatCodeGen::COND_TRANSLATE()
{
	out << 
		"	_widec = " << GET_KEY() << "\n"
		"	_keys = " << CS() << " << 1\n"
		"	_conds = " << CO() << "[" << CS() << "]\n"
		"	_slen = " << CSP() << "[" << CS() << "]\n"
		"	_cond = if ( _slen > 0 && \n" 
		"		     " << CK() << "[_keys] <= " << GET_WIDE_KEY() << " &&\n" 
		"		     " << GET_WIDE_KEY() << " <= " << CK() << "[_keys + 1]\n"
		"		   ) then \n"
		"			" << C() << "[ _conds + " << GET_WIDE_KEY() << " - " << CK() << "[_keys]" << " ]\n"
		"		else\n"
		"		       0\n"
		"		end\n";
	out <<
		"	case _cond \n";
	for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
		CondSpace *condSpace = csi;
		out << "	when " << condSpace->condSpaceId + 1 << "\n";
		out << TABS(2) << "_widec = " << "(" <<
				KEY(condSpace->baseKey) << " + (" << GET_KEY() << 
				" - " << KEY(keyOps->minKey) << "))\n";

		for ( CondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			out << TABS(2) << "if ( ";
			CONDITION( out, *csi );
			Size condValOffset = ((1 << csi.pos()) * keyOps->alphSize());
			out << 
				" ) then \n" <<
				TABS(3) << "  _widec += " << condValOffset << "\n"
				"end\n";
		}
	}

	out <<
		"	end # _cond switch \n";
}

std::ostream &RubyFlatCodeGen::CONDS()
{
	int totalTrans = 0;
	START_ARRAY_LINE();
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->condList != 0 ) {
			/* Walk the singles. */
			unsigned long long span = keyOps->span( st->condLowKey, st->condHighKey );
			for ( unsigned long long pos = 0; pos < span; pos++ ) {
				if ( st->condList[pos] != 0 )
					ARRAY_ITEM( INT( st->condList[pos]->condSpaceId + 1 ), ++totalTrans, false );
				else
					ARRAY_ITEM( INT( 0 ), ++totalTrans, false );
			}
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT( 0 ), ++totalTrans, true );
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyFlatCodeGen::COND_KEYS()
{
	START_ARRAY_LINE();
	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Emit just cond low key and cond high key. */
		ARRAY_ITEM( KEY( st->condLowKey ), ++totalTrans, false );
		ARRAY_ITEM( KEY( st->condHighKey ), ++totalTrans, false );
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT( 0 ), ++totalTrans, true );
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyFlatCodeGen::COND_KEY_SPANS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		unsigned long long span = 0;
		if ( st->condList != 0 )
			span = keyOps->span( st->condLowKey, st->condHighKey );
		ARRAY_ITEM( INT( span ), ++totalStateNum, false );
	}
	END_ARRAY_LINE();
	return out;
}


void RubyFlatCodeGen::GOTO( ostream &out, int gotoDest, bool inFinish )
{
	out << 
		"	begin\n"
		"		" << CS() << " = " << gotoDest << "\n"
		"		_break_again = true\n"
		"		break\n" // break _again
		"	end\n";
}

void RubyFlatCodeGen::CALL( ostream &out, int callDest, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		out << "begin\n";
		INLINE_LIST( out, prePushExpr, 0, false );
	}

	out <<
		"	begin\n"
		"		" << STACK() << "[" << TOP() << "] = " << CS() << "\n"
		"		" << TOP() << "+= 1\n"
		"		" << CS() << " = " << callDest << "\n"
		"		_break_again = true\n"
		"		break\n" // break _again
		"	end\n";

	if ( prePushExpr != 0 )
		out << "end\n";
}

void RubyFlatCodeGen::CALL_EXPR(ostream &out, InlineItem *ilItem, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		out << "begin\n";
		INLINE_LIST( out, prePushExpr, 0, false );
	}

	out <<
		"	begin\n"
		"		" << STACK() << "[" << TOP() << "] = " << CS() << "\n"
		"		" << TOP() << " += 1\n"
		"		" << CS() << " = (";
	INLINE_LIST( out, ilItem->children, targState, inFinish );
	out << ")\n";

	out << 
		"		_break_again = true\n"
		"		break\n" // break _again
		"	end\n";

	if ( prePushExpr != 0 )
		out << "end\n";
}

void RubyFlatCodeGen::RET( ostream &out, bool inFinish )
{
	out <<
		"	begin\n"
		"		" << TOP() << " -= 1\n"
		"		" << CS() << " = " << STACK() << "[" << TOP() << "]\n";

	if ( postPopExpr != 0 ) {
		out << "begin\n";
		INLINE_LIST( out, postPopExpr, 0, false );
		out << "end\n";
	}

	out <<
		"		_break_again = true\n"
		"		break\n" // break _again
		"	end\n";
}

void RubyFlatCodeGen::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << CS() << " = " << nextDest << ";";
}

void RubyFlatCodeGen::GOTO_EXPR( ostream &out, InlineItem *ilItem, bool inFinish )
{
	out << 
		"	begin\n"
		"		" << CS() << " = (";
	INLINE_LIST( out, ilItem->children, 0, inFinish );
	out << ")\n";
	out <<
		"		_break_again = true\n"
		"		break\n" // break _again
		"	end\n";
}

void RubyFlatCodeGen::NEXT_EXPR( ostream &ret, InlineItem *ilItem, bool inFinish )
{
	ret << CS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish );
	ret << ");";
}


void RubyFlatCodeGen::CURS( ostream &ret, bool inFinish )
{
	ret << "(_ps)";
}

void RubyFlatCodeGen::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << "(" << CS() << ")";
}

void RubyFlatCodeGen::BREAK( ostream &out, int targState )
{
	out << 
		"	begin\n"
		"		_break_resume = true\n"
		"		break\n"
		"	end\n";
}

int RubyFlatCodeGen::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	return act;
}

int RubyFlatCodeGen::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	return act;
}

int RubyFlatCodeGen::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	return act;
}

int RubyFlatCodeGen::TRANS_ACTION( RedTransAp *trans )
{
	/* If there are actions, emit them. Otherwise emit zero. */
	int act = 0;
	if ( trans->action != 0 )
		act = trans->action->location+1;
	return act;
}

void RubyFlatCodeGen::writeData()
{
	/* If there are any transtion functions then output the array. If there
	 * are none, don't bother emitting an empty array that won't be used. */
	if ( redFsm->anyActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActArrItem), A() );
		ACTIONS_ARRAY();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyConditions() ) {
		OPEN_ARRAY( WIDE_ALPH_TYPE(), CK() );
		COND_KEYS();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCondSpan), CSP() );
		COND_KEY_SPANS();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCond), C() );
		CONDS();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCondIndexOffset), CO() );
		COND_INDEX_OFFSET();
		CLOSE_ARRAY() <<
		"\n";
	}

	OPEN_ARRAY( WIDE_ALPH_TYPE(), K() );
	KEYS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxSpan), SP() );
	KEY_SPANS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxFlatIndexOffset), IO() );
	FLAT_INDEX_OFFSET();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndex), I() );
	INDICIES();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TT() );
	TRANS_TARGS();
	CLOSE_ARRAY() <<
	"\n";

	if ( redFsm->anyActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), TA() );
		TRANS_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyToStateActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), TSA() );
		TO_STATE_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyFromStateActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), FSA() );
		FROM_STATE_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyEofActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), EA() );
		EOF_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	STATE_IDS();
}

void RubyFlatCodeGen::writeEOF()
{
	if ( redFsm->anyEofActions() ) {
		out << 
			"	begin\n"
			"	" << "_acts = " << EA() << "[" << CS() << "]\n"
			"	_nacts = " << A() << "[_acts]\n" << 
			"	_acts += 1\n"
			"	while ( _nacts > 0 ) \n"
			"		_nacts -= 1\n"
			"		_acts += 1\n"
			"		case ( "<< A() << "[_acts-1] ) \n";
		EOF_ACTION_SWITCH();
		out <<
			"		end # eof action switch \n"
			"	end\n"
			"	end\n"
			"\n";
	}
}

void RubyFlatCodeGen::writeExec()
{
	out << 
		"begin # ragel flat\n"
		"	_slen, _trans, _keys, _inds";
	if ( redFsm->anyRegCurStateRef() )
		out << ", _ps";
	if ( redFsm->anyConditions() )
		out << ", _cond, _conds, _widec";
	if ( redFsm->anyToStateActions() || redFsm->anyRegActions() 
			|| redFsm->anyFromStateActions() )
		out << ", _acts, _nacts";
	
	out << " = nil\n";
	
	if ( hasEnd ) 
		out << "	if " << P() << " != " << PE() << " # pe guard\n";

	if ( redFsm->errState != 0 ) 
		out << "	if " << CS() << " != " << redFsm->errState->id << " # errstate guard\n";


	
	out << /* Open the _resume loop. */
		"	while true # _resume loop \n"
		"		_break_resume = false\n";	
	out << /* Open the _again loop. */
		"	begin\n"
		"		_break_again = false\n";

	if ( redFsm->anyFromStateActions() ) {
		out << 
			"	_acts = " << FSA() << "[" << CS() << "]\n"
			"	_nacts = " << A() << "[_acts]\n"
			"	_acts += 1\n"
			"	while _nacts > 0\n"
			"		_nacts -= 1\n"
			"		_acts += 1\n"
			"		case " << A() << "[_acts - 1]\n";
		FROM_STATE_ACTION_SWITCH();
		out <<
			"		end # from state action switch\n"
			"	end\n"
			"	break if _break_again\n";
	}

	if ( redFsm->anyConditions() )
		COND_TRANSLATE();
	
	LOCATE_TRANS();

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << CS() << "\n";

	out << "	" << CS() << " = " << TT() << "[_trans]\n";

	if ( redFsm->anyRegActions() ) {
		/* break _again */
		out << 
			"	break if " << TA() << "[_trans] == 0\n"
			"	_acts = " << TA() << "[_trans]\n"
			"	_nacts = " << A() << "[_acts]\n"
			"	_acts += 1\n"
			"	while _nacts > 0\n"
			"		_nacts -= 1\n"
			"		_acts += 1\n"
			"		case " << A() << "[_acts - 1]\n";
		ACTION_SWITCH();
		out <<
			"		end # action switch\n"
			"	end\n";

			/* Not necessary as long as there is no code between here and the
			 * end while false. */
			// "break if _break_again\n";
	}

	
	out << /* Close the _again loop. */
		"	end while false # _again loop\n"
		"	break if _break_resume\n";
	
	if ( redFsm->anyToStateActions() ) {
		out <<
			"	_acts = " << TSA() << "["  << CS() << "]\n"
			"	_nacts = " << A() << "[_acts]\n"
			"	_acts += 1\n"
			"	while _nacts > 0\n"
			"		_nacts -= 1\n"
			"		_acts += 1\n"
			"		case " << A() << "[_acts - 1]\n";
		TO_STATE_ACTION_SWITCH();
		out <<
			"		end # to state action switch\n"
			"	end\n";
	}

	if ( redFsm->errState != 0 ) 
		out << "	break if " << CS() << " == " << redFsm->errState->id << "\n";

	out << "	" << P() << " += 1\n";

	if ( hasEnd )
		out << "	break if " << P() << " == " << PE() << "\n";

	
	out << /* Close the _resume loop. */
		"	end # _resume loop\n";

	if ( redFsm->errState != 0 ) 
		out << "	end # errstate guard\n";

	
	if ( hasEnd )
		out << "	end # pe guard\n";

	out << "end # ragel flat";
}


/*
 * Local Variables:
 * mode: c++
 * indent-tabs-mode: 1
 * c-file-style: "bsd"
 * End:
 */
