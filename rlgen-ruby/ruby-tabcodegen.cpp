/*
 *  Copyright 2007 Victor Hugo Borja <vic@rubyforge.org>
 *            2007 Adrian Thurston <thurston@cs.queensu.ca>
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

#include <iomanip>
#include <sstream>
#include "redfsm.h"
#include "gendata.h"
#include "rlgen-ruby.h"
#include "ruby-tabcodegen.h"

using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;



void RubyTabCodeGen::GOTO( ostream &out, int gotoDest, bool inFinish )
{
	out << 
		"	begin\n"
		"		" << CS() << " = " << gotoDest << "\n"
		"		_break_again = true\n"
		"		break\n" // break _again
		"	end\n";
}

void RubyTabCodeGen::GOTO_EXPR( ostream &out, InlineItem *ilItem, bool inFinish )
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

void RubyTabCodeGen::CALL( ostream &out, int callDest, int targState, bool inFinish )
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

void RubyTabCodeGen::CALL_EXPR(ostream &out, InlineItem *ilItem, int targState, bool inFinish )
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

void RubyTabCodeGen::RET( ostream &out, bool inFinish )
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

void RubyTabCodeGen::BREAK( ostream &out, int targState )
{
	out << 
		"	begin\n"
		"		_break_resume = true\n"
		"		break\n"
		"	end\n";
}

void RubyTabCodeGen::COND_TRANSLATE()
{
	out <<
		"	_widec = " << GET_KEY() << "\n"
		"	_keys = " << CO() << "[" << CS() << "]*2\n"
		"	_klen = " << CL() << "[" << CS() << "]\n"
		"	if _klen > 0\n"
		"		_lower = _keys\n"
		"		_upper = _keys + (_klen<<1) - 2\n"
		"		loop do\n"
		"			break if _upper < _lower\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1)\n"
		"			if " << GET_WIDE_KEY() << " < " << CK() << "[_mid]\n"
		"				_upper = _mid - 2\n"
		"			elsif " << GET_WIDE_KEY() << " > " << CK() << "[_mid+1]\n"
		"				_lower = _mid + 2\n"
		"			else\n"
		"				case " << C() << "[" << CO() << "[" << CS() << "]"
							" + ((_mid - _keys)>>1)]\n";

	for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
		CondSpace *condSpace = csi;
		out << "	when " << condSpace->condSpaceId << ":" ;
		out << "	_widec = " << KEY(condSpace->baseKey) << 
				"+ (" << GET_KEY() << " - " << KEY(keyOps->minKey) << ")\n";

		for ( CondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			Size condValOffset = ((1 << csi.pos()) * keyOps->alphSize());
			out << "	_widec += " << condValOffset << " if ( ";
			CONDITION( out, *csi );
			out << " )\n";
		}
	}

	out <<
		"				end # case\n"
		"			end\n"
		"		end # loop\n"
		"	end\n";
}


void RubyTabCodeGen::LOCATE_TRANS()
{
	out <<
		"	_keys = " << KO() << "[" << CS() << "]\n"
		"	_trans = " << IO() << "[" << CS() << "]\n"
		"	_klen = " << SL() << "[" << CS() << "]\n"
		"	_break_match = false\n"
		"	\n"
		"	begin\n"
		"	  if _klen > 0\n"
		"	     _lower = _keys\n"
		"	     _upper = _keys + _klen - 1\n"
		"\n"
		"	     loop do\n"
		"	        break if _upper < _lower\n"
		"	        _mid = _lower + ( (_upper - _lower) >> 1 )\n"
		"\n"
		"	        if " << GET_WIDE_KEY() << " < " << K() << "[_mid]\n"
		"	           _upper = _mid - 1\n"
		"	        elsif " << GET_WIDE_KEY() << " > " << K() << "[_mid]\n"
		"	           _lower = _mid + 1\n"
		"	        else\n"
		"	           _trans += (_mid - _keys)\n"
		"	           _break_match = true\n"
		"	           break\n"
		"	        end\n"
		"	     end # loop\n"
		"	     break if _break_match\n"
		"	     _keys += _klen\n"
		"	     _trans += _klen\n"
		"	  end"
		"\n"
		"	  _klen = " << RL() << "[" << CS() << "]\n"
		"	  if _klen > 0\n"
		"	     _lower = _keys\n"
		"	     _upper = _keys + (_klen << 1) - 2\n"
		"	     loop do\n"
		"	        break if _upper < _lower\n"
		"	        _mid = _lower + (((_upper-_lower) >> 1) & ~1)\n"
		"	        if " << GET_WIDE_KEY() << " < " << K() << "[_mid]\n"
		"	          _upper = _mid - 2\n"
		"	        elsif " << GET_WIDE_KEY() << " > " << K() << "[_mid+1]\n"
		"	          _lower = _mid + 2\n"
		"	        else\n"
		"	          _trans += ((_mid - _keys) >> 1)\n"
		"	          _break_match = true\n"
		"	          break\n"
		"	        end\n"
		"	     end # loop\n"
		"	     break if _break_match\n"
		"	     _trans += _klen\n"
		"	  end\n"
		"	end while false\n";
}

void RubyTabCodeGen::writeExec()
{
	out << "begin\n"
		<< "	_klen, _trans, _keys";

	if ( redFsm->anyRegCurStateRef() )
		out << ", _ps";
	if ( redFsm->anyConditions() ) 
		out << ", _widec";
	if ( redFsm->anyToStateActions() || redFsm->anyRegActions() 
			|| redFsm->anyFromStateActions() )
		out << ", _acts, _nacts";

	out << " = nil\n";

	if ( hasEnd ) 
		out << "	if " << P() << " != " << PE() << "\n";

	if ( redFsm->errState != 0 ) 
		out << "	if " << CS() << " != " << redFsm->errState->id << "\n";

	/* Open the _resume loop. */
	out << "	while true\n"
		<< "	_break_resume = false\n";

	/* Open the _again loop. */
	out << "	begin\n"
		<< "	_break_again = false\n";

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

	if ( useIndicies )
		out << "	_trans = " << I() << "[_trans]\n";

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

	/* Close the again loop. */
	out << "	end while false\n";
	out << "	break if _break_resume\n";

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

	/* Close the resume loop. */
	out << "	end\n";

	/* The if guarding on the error state. */
	if ( redFsm->errState != 0 ) 
		out << "	end\n";

	/* The if guarding on empty string. */
	if ( hasEnd ) 
		out << "	end\n";

	/* Wrapping the execute block. */
	out << "	end\n";
}

void RubyTabCodeGen::writeEOF()
{
	if ( redFsm->anyEofActions() ) {
		out << 
			"	_acts = " << EA() << "[" << CS() << "]\n"
			"	_nacts = " << " " << A() << "[_acts]\n"
			"	_acts += 1\n"
			"	while _nacts > 0\n"
			"		_nacts -= 1\n"
			"		_acts += 1\n"
			"		case " << A() << "[_acts - 1]\n";
		EOF_ACTION_SWITCH() <<
			"		end # eof action switch\n"
			"	end\n";
	}
}



std::ostream &RubyTabCodeGen::FROM_STATE_ACTION_SWITCH() 
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action */
			out << "			when " << act->actionId << ":\n";
			ACTION( out, act, 0, false );
		}
	}

	genLineDirective( out );
	return out;
}


std::ostream &RubyTabCodeGen::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "when " << act->actionId << "\n";
			ACTION( out, act, 0, false );
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &RubyTabCodeGen::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "when " << act->actionId << ":\n";
			ACTION( out, act, 0, true );
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &RubyTabCodeGen::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "when " << act->actionId << ":\n";
			ACTION( out, act, 0, false );
		}
	}

	genLineDirective( out );
	return out;
}


void RubyTabCodeGen::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << CS() << " = " << nextDest << ";";
}

void RubyTabCodeGen::NEXT_EXPR( ostream &ret, InlineItem *ilItem, bool inFinish )
{
	ret << CS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish );
	ret << ");";
}


int RubyTabCodeGen::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	return act;
}

int RubyTabCodeGen::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	return act;
}

int RubyTabCodeGen::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	return act;
}


std::ostream &RubyTabCodeGen::COND_OFFSETS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0, curKeyOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the key offset. */
		ARRAY_ITEM( INT(curKeyOffset), ++totalStateNum, st.last() );

		/* Move the key offset ahead. */
		curKeyOffset += st->stateCondList.length();
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::KEY_OFFSETS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0, curKeyOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the key offset. */
		ARRAY_ITEM( INT(curKeyOffset), ++totalStateNum, st.last() );

		/* Move the key offset ahead. */
		curKeyOffset += st->outSingle.length() + st->outRange.length()*2;
	}
	END_ARRAY_LINE();
	return out;
}


std::ostream &RubyTabCodeGen::INDEX_OFFSETS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0, curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		ARRAY_ITEM( INT(curIndOffset), ++totalStateNum, st.last() );

		/* Move the index offset ahead. */
		curIndOffset += st->outSingle.length() + st->outRange.length();
		if ( st->defTrans != 0 )
			curIndOffset += 1;
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::COND_LENS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		ARRAY_ITEM( INT(st->stateCondList.length()), ++totalStateNum, st.last() );
	}
	END_ARRAY_LINE();
	return out;
}


std::ostream &RubyTabCodeGen::SINGLE_LENS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		ARRAY_ITEM( INT(st->outSingle.length()), ++totalStateNum, st.last() );
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::RANGE_LENS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Emit length of range index. */
		ARRAY_ITEM( INT(st->outRange.length()), ++totalStateNum, st.last() );
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::TO_STATE_ACTIONS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		ARRAY_ITEM( INT(TO_STATE_ACTION(st)), ++totalStateNum, st.last() );
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::FROM_STATE_ACTIONS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		ARRAY_ITEM( INT(FROM_STATE_ACTION(st)), ++totalStateNum, st.last() );
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::EOF_ACTIONS()
{
	START_ARRAY_LINE();
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		ARRAY_ITEM( INT(EOF_ACTION(st)), ++totalStateNum, st.last() );
	}
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::COND_KEYS()
{
	START_ARRAY_LINE();
	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the state's transitions. */
		for ( StateCondList::Iter sc = st->stateCondList; sc.lte(); sc++ ) {
			/* Lower key. */
			ARRAY_ITEM( KEY( sc->lowKey ), ++totalTrans, false );
			ARRAY_ITEM( KEY( sc->highKey ), ++totalTrans, false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), ++totalTrans, true );
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::COND_SPACES()
{
	START_ARRAY_LINE();
	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the state's transitions. */
		for ( StateCondList::Iter sc = st->stateCondList; sc.lte(); sc++ ) {
			/* Cond Space id. */
			ARRAY_ITEM( KEY( sc->condSpace->condSpaceId ), ++totalTrans, false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), ++totalTrans, true );
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::KEYS()
{
	START_ARRAY_LINE();
	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			ARRAY_ITEM( KEY( stel->lowKey ), ++totalTrans, false );
		}

		/* Loop the state's transitions. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			/* Lower key. */
			ARRAY_ITEM( KEY( rtel->lowKey ), ++totalTrans, false );

			/* Upper key. */
			ARRAY_ITEM( KEY( rtel->highKey ), ++totalTrans, false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), ++totalTrans, true );
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::INDICIES()
{
	int totalTrans = 0;
	START_ARRAY_LINE();
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			ARRAY_ITEM( KEY( stel->value->id ), ++totalTrans, false );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			ARRAY_ITEM( KEY( rtel->value->id ), ++totalTrans, false );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			ARRAY_ITEM( KEY( st->defTrans->id ), ++totalTrans, false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), ++totalTrans, true );
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::TRANS_TARGS()
{
	int totalTrans = 0;
	START_ARRAY_LINE();
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			ARRAY_ITEM( KEY( trans->targ->id ), ++totalTrans, false );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			ARRAY_ITEM( KEY( trans->targ->id ), ++totalTrans, false );
		}

		/* The state's default target state. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			ARRAY_ITEM( KEY( trans->targ->id ), ++totalTrans, false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), ++totalTrans, true );
	END_ARRAY_LINE();
	return out;
}


std::ostream &RubyTabCodeGen::TRANS_ACTIONS()
{
	int totalTrans = 0;
	START_ARRAY_LINE();
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			ARRAY_ITEM( INT(TRANS_ACTION( trans )), ++totalTrans, false );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			ARRAY_ITEM( INT(TRANS_ACTION( trans )), ++totalTrans, false );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			ARRAY_ITEM( INT(TRANS_ACTION( trans )), ++totalTrans, false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), ++totalTrans, true );
	END_ARRAY_LINE();
	return out;
}

std::ostream &RubyTabCodeGen::TRANS_TARGS_WI()
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
		ARRAY_ITEM( INT(trans->targ->id), ++totalStates, ( t >= redFsm->transSet.length()-1 ) );
	}
	END_ARRAY_LINE();
	delete[] transPtrs;
	return out;
}


std::ostream &RubyTabCodeGen::TRANS_ACTIONS_WI()
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
		ARRAY_ITEM( INT(TRANS_ACTION( trans )), ++totalAct, ( t >= redFsm->transSet.length()-1 ) );
	}
	END_ARRAY_LINE();
	delete[] transPtrs;
	return out;
}


void RubyTabCodeGen::writeData()
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
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCondOffset), CO() );
		COND_OFFSETS();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCondLen), CL() );
		COND_LENS();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( WIDE_ALPH_TYPE(), CK() );
		COND_KEYS();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCondSpaceId), C() );
		COND_SPACES();
		CLOSE_ARRAY() <<
		"\n";
	}

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxKeyOffset), KO() );
	KEY_OFFSETS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( WIDE_ALPH_TYPE(), K() );
	KEYS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxSingleLen), SL() );
	SINGLE_LENS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxRangeLen), RL() );
	RANGE_LENS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndexOffset), IO() );
	INDEX_OFFSETS();
	CLOSE_ARRAY() <<
	"\n";

	if ( useIndicies ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndex), I() );
		INDICIES();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TT() );
		TRANS_TARGS_WI();
		CLOSE_ARRAY() <<
		"\n";

		if ( redFsm->anyActions() ) {
			OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), TA() );
			TRANS_ACTIONS_WI();
			CLOSE_ARRAY() <<
			"\n";
		}
	}
	else {
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

/*
 Local Variables:
 mode: c++
 indent-tabs-mode: 1
 c-file-style: "bsd"
 End:
 */
