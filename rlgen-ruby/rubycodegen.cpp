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

#include "rlgen-ruby.h"
#include "rubycodegen.h"
#include "redfsm.h"
#include "gendata.h"
#include <iomanip>
#include <sstream>

/* Integer array line length. */
#define IALL 8

using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;

void lineDirective( ostream &out, char *fileName, int line )
{
	/* Write a comment containing line info. */
	out << "# line " << line  << " \"";
	for ( char *pc = fileName; *pc != 0; pc++ ) {
		if ( *pc == '\\' )
			out << "\\\\";
		else
			out << *pc;
	}
	out << "\"\n";
}

void genLineDirective( ostream &out )
{
	std::streambuf *sbuf = out.rdbuf();
	output_filter *filter = static_cast<output_filter*>(sbuf);
	lineDirective( out, filter->fileName, filter->line + 1 );
}

void RubyCodeGen::GOTO( ostream &out, int gotoDest, bool inFinish )
{
	out << INDENT_U() << "begin"
		<< INDENT_S() <<     CS() << " = " << gotoDest
		<< INDENT_S() <<     "_again.call " << CTRL_FLOW()
		<< INDENT_D() << "end";
}

void RubyCodeGen::GOTO_EXPR( ostream &out, InlineItem *ilItem, bool inFinish )
{
	out << INDENT_U() << "begin"
		<< INDENT_S() <<    CS() << " = (";
	INLINE_LIST( out, ilItem->children, 0, inFinish );
	out << ")"
		<< INDENT_S() <<    "_again.call " << CTRL_FLOW()
		<< INDENT_D() << "end";
}

void RubyCodeGen::CALL( ostream &out, int callDest, int targState, bool inFinish )
{
	out << INDENT_U() << "begin" 
		<< INDENT_S() <<   STACK() << "[" << TOP() << "] = " << CS() 
		<< INDENT_S() <<   TOP() << "+= 1" 
		<< INDENT_S() <<   CS() << " = " << callDest 
		<< INDENT_S() <<   "_again.call " << CTRL_FLOW() 
		<< INDENT_D() << "end";
}

void RubyCodeGen::CALL_EXPR(ostream &out, InlineItem *ilItem, int targState, bool inFinish )
{
	out << INDENT_U() << "begin" 
		<< INDENT_S() <<   STACK() << "[" << TOP() << "] = " << CS() 
		<< INDENT_S() <<   TOP() << " += 1" 
		<< INDENT_S() <<   CS() << " = (";
	INLINE_LIST( out, ilItem->children, targState, inFinish );
	out << ")" 
		<< INDENT_S() <<   "_again.call " << CTRL_FLOW() 
		<< INDENT_D() << "end";
}

void RubyCodeGen::RET( ostream &out, bool inFinish )
{
	out << INDENT_U() << "begin" 
		<< INDENT_S() <<   TOP() << " -= 1" 
		<< INDENT_S() <<   CS() << " = " << STACK() << "[" << TOP() << "]" 
		<< INDENT_S() <<   "_again.call " << CTRL_FLOW() 
		<< INDENT_D() << "end";
}

void RubyCodeGen::BREAK( ostream &out, int targState )
{
	out << "_out.call " << CTRL_FLOW();
}

void RubyCodeGen::COND_TRANSLATE()
{
	out << INDENT_S() << "_widec = " << GET_KEY() 
		<< INDENT_S() << "_keys = " << CO() << "[" << CS() << "]*2" 
		<< INDENT_S() << "_klen = " << CL() << "[" << CS() << "]" 
		<< INDENT_U() << "if _klen > 0" 
		<< INDENT_S() <<   "_lower = _keys" 
		<< INDENT_S() <<   "_upper = _keys + (_klen<<1) - 2" 
		<< INDENT_U() <<   "loop do" 
		<< INDENT_S() <<      "break if _upper < _lower"  
		<< INDENT_S() <<      "_mid = _lower + (((_upper-_lower) >> 1) & ~1)" 
		<< INDENT_U() <<      "if " << GET_WIDE_KEY() << " < " << CK() << "[_mid]" 
		<< INDENT_O() <<         "_upper = _mid - 2" 
		<< INDENT_U() <<      "elsif " << GET_WIDE_KEY() << " > " << CK() << "[_mid]" 
		<< INDENT_O() <<         "_lower = _mid + 2" 
		<< INDENT_U() <<      "else" 
		<< INDENT_U() <<         "case " << C() << "[" << CO() << "[" << CS() << "]" 
		<<                      " + ((_mid - _keys)>>1)]" 
	;

	for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
		CondSpace *condSpace = csi;
		out << INDENT_U() << "when " << condSpace->condSpaceId << ":" ;
		out << INDENT_S() <<    "_widec = " << KEY(condSpace->baseKey)
			<< "+ (" << GET_KEY() << " - " << KEY(keyOps->minKey) << ")" ;

		for ( CondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			Size condValOffset = ((1 << csi.pos()) * keyOps->alphSize());
			out << INDENT_S() << "_widec += " << condValOffset << " if ( ";
			CONDITION( out, *csi );
			out << " )";
		}
	}

	out << INDENT_D() << "end # case" 
		<< INDENT_D() << "end" 
		<< INDENT_D() << "end # loop" 
		<< INDENT_D() << "end" 
	;
}

void RubyCodeGen::LOCATE_TRANS()
{
	out << INDENT_S() << "_keys = " << KO() << "[" << CS() << "]" 
		<< INDENT_S() << "_trans = " << IO() << "[" << CS() << "]" 
		<< INDENT_S() << "_klen = " << SL() << "[" << CS() << "]" 
		<< INDENT_S()
		<< INDENT_U() << "callcc do |_match|" 
		<< INDENT_U() <<    "if _klen > 0" 
		<< INDENT_S() <<       "_lower = _keys" 
		<< INDENT_S() <<       "_upper = _keys + _klen - 1" 
		<< INDENT_S()
		<< INDENT_U() <<       "loop do" 
		<< INDENT_S() <<          "break if _upper < _lower" 
		<< INDENT_S() <<          "_mid = _lower + ( (_upper - _lower) >> 1 )" 
		<< INDENT_S()
		<< INDENT_U() <<          "if " << GET_WIDE_KEY() << " < " << K() << "[_mid]" 
		<< INDENT_O() <<             "_upper = _mid - 1" 
		<< INDENT_U() <<          "elsif " << GET_WIDE_KEY() << " > " << K() << "[_mid]" 
		<< INDENT_O() <<             "_lower = _mid + 1" 
		<< INDENT_U() <<          "else" 
		<< INDENT_S() <<             "_trans += (_mid - _keys)" 
		<< INDENT_S() <<             "_match.call" 
		<< INDENT_D() <<          "end" 
		<< INDENT_D() <<       "end # loop" 
		<< INDENT_S() <<       "_keys += _klen" 
		<< INDENT_S() <<       "_trans += _klen" 
		<< INDENT_D() <<    "end" 
		<< INDENT_S()
		<< INDENT_S() <<    "_klen = " << RL() << "[" << CS() << "]" 
		<< INDENT_U() <<    "if _klen > 0" 
		<< INDENT_S() <<       "_lower = _keys" 
		<< INDENT_S() <<       "_upper = _keys + (_klen << 1) - 2" 
		<< INDENT_U() <<       "loop do" 
		<< INDENT_S() <<          "break if _upper < _lower"
		<< INDENT_S() <<          "_mid = _lower + (((_upper-_lower) >> 1) & ~1)" 
		<< INDENT_U() <<          "if " << GET_WIDE_KEY() << " < " << K() << "[_mid]" 
		<< INDENT_O() <<            "_upper = _mid - 2" 
		<< INDENT_U() <<          "elsif " << GET_WIDE_KEY() << " > " << K() << "[_mid+1]" 
		<< INDENT_O() <<            "_lower = _mid + 2" 
		<< INDENT_U() <<          "else" 
		<< INDENT_S() <<            "_trans += ((_mid - _keys) >> 1)" 
		<< INDENT_S() <<            "_match.call" 
		<< INDENT_D() <<          "end" 
		<< INDENT_D() <<       "end # loop" 
		<< INDENT_S() <<       "_trans += _klen" 
		<< INDENT_D() <<    "end" 
		<< INDENT_D() << "end # cc _match" ;
}

void RubyCodeGen::writeExec()
{
	out << INDENT_U() << "callcc do |_out|" 
		<< INDENT_S() <<    "_klen, _trans, _keys";

	if ( redFsm->anyRegCurStateRef() )
		out << ", _ps";
	if ( redFsm->anyConditions() ) 
		out << ", _widec";
	if ( redFsm->anyToStateActions() || redFsm->anyRegActions() 
			|| redFsm->anyFromStateActions() )
		out << ", _acts, _nacts";

	out << " = nil" ;

	if ( hasEnd ) 
		out << INDENT_S() << "_out.call if " << P() << " == " << PE() ;

	out << INDENT_S() << "_resume = nil" 
		<< INDENT_S() << "callcc { |_cc| _resume = _cc }" ;

	if ( redFsm->errState != 0) 
		out << INDENT_S() << "_out.call if " << CS() << " == " << redFsm->errState->id ;

	if ( redFsm->anyRegActions() || redFsm->anyActionGotos() || 
			redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << INDENT_U() << "callcc do |_again|" ;

	if ( redFsm->anyFromStateActions() ) {
		out << INDENT_S() << "_acts = " << FSA() << "[" << CS() << "]" 
			<< INDENT_S() << "_nacts = " << A() << "[_acts]" 
			<< INDENT_S() << "_acts += 1" 
			<< INDENT_U() << "while _nacts > 0" 
			<< INDENT_S() <<   "_nacts -= 1" 
			<< INDENT_S() <<   " _acts += 1" 
			<< INDENT_U() <<   "case " << A() << "[_acts - 1]" ;
		FROM_STATE_ACTION_SWITCH()
			<< INDENT_D() <<   "end # from state action switch" 
			<< INDENT_D() << "end" 
			<< INDENT_S();
	}

	if ( redFsm->anyConditions() )
		COND_TRANSLATE();

	LOCATE_TRANS();

	if ( redFsm->anyRegCurStateRef() )
		out << INDENT_S() << "_ps = " << CS() ;

	if ( useIndicies )
		out << INDENT_S() << "_trans = " << I() << "[_trans]" ;

	out << INDENT_S() << CS() << " = " << TT() << "[_trans]" ;

	if ( redFsm->anyRegActions() ) {
		out << INDENT_S() << "_again.call if " << TA() << "[_trans] == 0" 
			<< INDENT_S()
			<< INDENT_S() << "_acts = " << TA() << "[_trans]" 
			<< INDENT_S() << "_nacts = " << A() << "[_acts]" 
			<< INDENT_S() << "_acts += 1" 
			<< INDENT_U() << "while _nacts > 0" 
			<< INDENT_S() <<   "_nacts -= 1" 
			<< INDENT_S() <<   "_acts += 1" 
			<< INDENT_U() <<   "case " << A() << "[_acts - 1]" ;
		ACTION_SWITCH()
			<< INDENT_D() << "end # action switch"
			<< INDENT_D() << "end"
			<< INDENT_S();
	}

	if ( redFsm->anyRegActions() || redFsm->anyActionGotos() || 
			redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << INDENT_D() << "end # cc _again";

	if ( redFsm->anyToStateActions() ) {
		out << INDENT_S() << "_acts = " << TSA() << "["  << CS() << "]"  
			<< INDENT_S() << "_nacts = " << A() << "[_acts]" 
			<< INDENT_S() << "_acts += 1" 
			<< INDENT_U() << "while _nacts > 0" 
			<< INDENT_S() <<   "_nacts -= 1" 
			<< INDENT_S() <<   "_acts += 1" 
			<< INDENT_U() <<   "case " << A() << "[_acts - 1]" ;
		TO_STATE_ACTION_SWITCH()
			<< INDENT_D() <<     "end # to state action switch"
			<< INDENT_D() << "end" 
			<< INDENT_S();
	}

	out << INDENT_S() << P() << " += 1" ;

	if ( hasEnd )
		out << INDENT_S() << "_resume.call if p != pe";

	out << INDENT_D() << "end # cc _out" ;          
}

void RubyCodeGen::writeEOF()
{
	if ( redFsm->anyEofActions() ) {
		out << INDENT_S() << "_acts = " << EA() << "[" << CS() << "]" 
			<< INDENT_S() << "_nacts = " << " " << A() << "[_acts]" 
			<< INDENT_S() << "_acts += 1" 
			<< INDENT_U() << "while _nacts > 0" 
			<< INDENT_S() <<    "_nacts -= 1" 
			<< INDENT_S() <<    "_acts += 1" 
			<< INDENT_S() <<    "case " << A() << "[_acts - 1]" ;
		EOF_ACTION_SWITCH()
			<< INDENT_D() << "end # eof action switch" 
			<< INDENT_D() << "end" 
			<< INDENT_S();
	}
}

std::ostream &RubyCodeGen::FROM_STATE_ACTION_SWITCH() 
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action */
			out << INDENT_S() << "when " << act->actionId << ":" ;
			ACTION( out, act, 0, false );
		}
	}

	genLineDirective( out );
	return out;
}


std::ostream &RubyCodeGen::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << INDENT_S() << "when " << act->actionId << ":" ;
			ACTION( out, act, 0, false );
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &RubyCodeGen::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << INDENT_S() << "when " << act->actionId << ":" ;
			ACTION( out, act, 0, true );
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &RubyCodeGen::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << INDENT_S() << "when " << act->actionId << ":" ;
			ACTION( out, act, 0, false );
		}
	}

	genLineDirective( out );
	return out;
}


void RubyCodeGen::writeInit()
{
	out << INDENT_U() << "begin";

	if ( writeCS )
		out << INDENT_S() <<   CS() << " = " << START();

	/* If there are any calls, then the stack top needs initialization. */
	if ( redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << INDENT_S() << TOP() << " = 0";

	if ( hasLongestMatch ) {
		out << INDENT_S() << TOKSTART() << " = " << NULL_ITEM() 
			<< INDENT_S() << TOKEND() << " = " << NULL_ITEM()
			<< INDENT_S() << ACT() << " = 0"
			<< INDENT_S();
	}
	out << INDENT_D() << "end";
}

std::ostream &RubyCodeGen::OPEN_ARRAY( string type, string name )
{
	out << "class << self" << endl
		<< INDENT(1) << "attr_accessor :" << name << endl
		<< INDENT(1) << "private :" << name << ", :" << name << "=" << endl
		<< "end" << endl
		<< "self." << name << " = [" << endl;
	return out;
}

std::ostream &RubyCodeGen::CLOSE_ARRAY()
{
	return out << "]" << endl;
}

std::ostream &RubyCodeGen::STATIC_VAR( string type, string name )
{
	out << "class << self" << endl
		<< INDENT(1) << "attr_accessor :" << name << endl
		<< "end" << endl
		<< "self." << name;
	return out;
}

string RubyCodeGen::ARR_OFF( string ptr, string offset )
{
	return ptr + " + " + offset;
}

string RubyCodeGen::NULL_ITEM()
{
	return "nil";
}

string RubyCodeGen::GET_KEY()
{
	ostringstream ret;
	if ( getKeyExpr != 0 ) { 
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, getKeyExpr, 0, false );
		ret << ")";
	}
	else {
		/* Expression for retrieving the key, use simple dereference. */
		ret << "data[" << P() << "]";
	}
	return ret.str();
}

string RubyCodeGen::CTRL_FLOW()
{
	return "if (true)";
}

void RubyCodeGen::ACTION( ostream &ret, Action *action, int targState, bool inFinish )
{
	/* Write the preprocessor line info for going into the source file. */
	lineDirective( ret, sourceFileName, action->loc.line );

	/* Write the block and close it off. */
	ret << " begin " << endl << INDENT(1);
	INLINE_LIST( ret, action->inlineList, targState, inFinish );
	ret << " end\n";
	lineDirective( ret, sourceFileName, action->loc.line );
	ret << endl;
}

string RubyCodeGen::INDENT(int level)
{
	string result = "\n";
	while ( level-- > 0 )
		result += "  "; /* The convention in ruby is 2 spaces per level */
	return result;
}


void RubyCodeGen::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << CS() << " = " << nextDest << ";";
}

void RubyCodeGen::NEXT_EXPR( ostream &ret, InlineItem *ilItem, bool inFinish )
{
	ret << CS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish );
	ret << ");";
}

void RubyCodeGen::EXEC( ostream &ret, InlineItem *item, int targState, int inFinish )
{
	/* The parser gives fexec two children. The double brackets are for D
	 * code. If the inline list is a single word it will get interpreted as a
	 * C-style cast by the D compiler. */
	ret << " begin " << P() << " = ((";
	INLINE_LIST( ret, item->children, targState, inFinish );
	ret << "))-1; end\n";
}

void RubyCodeGen::EXECTE( ostream &ret, InlineItem *item, int targState, int inFinish )
{
	/* Tokend version of exec. */

	/* The parser gives fexec two children. The double brackets are for D
	 * code. If the inline list is a single word it will get interpreted as a
	 * C-style cast by the D compiler. */
	ret << " begin " << TOKEND() << " = ((";
	INLINE_LIST( ret, item->children, targState, inFinish );
	ret << ")); end\n";
}

/* Write out an inline tree structure. Walks the list and possibly calls out
 * to virtual functions than handle language specific items in the tree. */
void RubyCodeGen::INLINE_LIST( ostream &ret, InlineList *inlineList, 
		int targState, bool inFinish )
{
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case InlineItem::Text:
			ret << item->data;
			break;
		case InlineItem::Goto:
			GOTO( ret, item->targState->id, inFinish );
			break;
		case InlineItem::Call:
			CALL( ret, item->targState->id, targState, inFinish );
			break;
		case InlineItem::Next:
			NEXT( ret, item->targState->id, inFinish );
			break;
		case InlineItem::Ret:
			RET( ret, inFinish );
			break;
		case InlineItem::PChar:
			ret << P();
			break;
		case InlineItem::Char:
			ret << GET_KEY();
			break;
		case InlineItem::Hold:
			ret << P() << " = " << P() << " - 1;";
			break;
		case InlineItem::Exec:
			EXEC( ret, item, targState, inFinish );
			break;
		case InlineItem::HoldTE:
			ret << TOKEND() << " = " << TOKEND() << " - 1;";
			break;
		case InlineItem::ExecTE:
			EXECTE( ret, item, targState, inFinish );
			break;
		case InlineItem::Curs:
			ret << "(_ps)";
			break;
		case InlineItem::Targs:
			ret << "(" << CS() << ")";
			break;
		case InlineItem::Entry:
			ret << item->targState->id;
			break;
		case InlineItem::GotoExpr:
			GOTO_EXPR( ret, item, inFinish );
			break;
		case InlineItem::CallExpr:
			CALL_EXPR( ret, item, targState, inFinish );
			break;
		case InlineItem::NextExpr:
			NEXT_EXPR( ret, item, inFinish );
			break;
		case InlineItem::LmSwitch:
			LM_SWITCH( ret, item, targState, inFinish );
			break;
		case InlineItem::LmSetActId:
			SET_ACT( ret, item );
			break;
		case InlineItem::LmSetTokEnd:
			SET_TOKEND( ret, item );
			break;
		case InlineItem::LmGetTokEnd:
			GET_TOKEND( ret, item );
			break;
		case InlineItem::LmInitTokStart:
			INIT_TOKSTART( ret, item );
			break;
		case InlineItem::LmInitAct:
			INIT_ACT( ret, item );
			break;
		case InlineItem::LmSetTokStart:
			SET_TOKSTART( ret, item );
			break;
		case InlineItem::SubAction:
			SUB_ACTION( ret, item, targState, inFinish );
			break;
		case InlineItem::Break:
			BREAK( ret, targState );
			break;
		}
	}
}

string RubyCodeGen::DATA_PREFIX()
{
	if ( dataPrefix )
		return FSM_NAME() + "_";
	return "";
}

/* Emit the alphabet data type. */
string RubyCodeGen::ALPH_TYPE()
{
	string ret = keyOps->alphType->data1;
	if ( keyOps->alphType->data2 != 0 ) {
		ret += " ";
		ret += + keyOps->alphType->data2;
	}
	return ret;
}

/* Emit the alphabet data type. */
string RubyCodeGen::WIDE_ALPH_TYPE()
{
	string ret;
	if ( redFsm->maxKey <= keyOps->maxKey )
		ret = ALPH_TYPE();
	else {
		long long maxKeyVal = redFsm->maxKey.getLongLong();
		HostType *wideType = keyOps->typeSubsumes( keyOps->isSigned, maxKeyVal );
		assert( wideType != 0 );

		ret = wideType->data1;
		if ( wideType->data2 != 0 ) {
			ret += " ";
			ret += wideType->data2;
		}
	}
	return ret;
}

/* Determine if we should use indicies or not. */
void RubyCodeGen::calcIndexSize()
{
	int sizeWithInds = 0, sizeWithoutInds = 0;

	/* Calculate cost of using with indicies. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		int totalIndex = st->outSingle.length() + st->outRange.length() + 
				(st->defTrans == 0 ? 0 : 1);
		sizeWithInds += arrayTypeSize(redFsm->maxIndex) * totalIndex;
	}
	sizeWithInds += arrayTypeSize(redFsm->maxState) * redFsm->transSet.length();
	if ( redFsm->anyActions() )
		sizeWithInds += arrayTypeSize(redFsm->maxActionLoc) * redFsm->transSet.length();

	/* Calculate the cost of not using indicies. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		int totalIndex = st->outSingle.length() + st->outRange.length() + 
				(st->defTrans == 0 ? 0 : 1);
		sizeWithoutInds += arrayTypeSize(redFsm->maxState) * totalIndex;
		if ( redFsm->anyActions() )
			sizeWithoutInds += arrayTypeSize(redFsm->maxActionLoc) * totalIndex;
	}

	/* If using indicies reduces the size, use them. */
	useIndicies = sizeWithInds < sizeWithoutInds;
}

int RubyCodeGen::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	return act;
}

int RubyCodeGen::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	return act;
}

int RubyCodeGen::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	return act;
}


int RubyCodeGen::TRANS_ACTION( RedTransAp *trans )
{
	/* If there are actions, emit them. Otherwise emit zero. */
	int act = 0;
	if ( trans->action != 0 )
		act = trans->action->location+1;
	return act;
}

std::ostream &RubyCodeGen::COND_OFFSETS()
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

std::ostream &RubyCodeGen::KEY_OFFSETS()
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


std::ostream &RubyCodeGen::INDEX_OFFSETS()
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

std::ostream &RubyCodeGen::COND_LENS()
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


std::ostream &RubyCodeGen::SINGLE_LENS()
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

std::ostream &RubyCodeGen::RANGE_LENS()
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

std::ostream &RubyCodeGen::TO_STATE_ACTIONS()
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

std::ostream &RubyCodeGen::FROM_STATE_ACTIONS()
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

std::ostream &RubyCodeGen::EOF_ACTIONS()
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

std::ostream &RubyCodeGen::COND_KEYS()
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

std::ostream &RubyCodeGen::COND_SPACES()
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

std::ostream &RubyCodeGen::KEYS()
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

std::ostream &RubyCodeGen::INDICIES()
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

std::ostream &RubyCodeGen::TRANS_TARGS()
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


std::ostream &RubyCodeGen::TRANS_ACTIONS()
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

std::ostream &RubyCodeGen::TRANS_TARGS_WI()
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


std::ostream &RubyCodeGen::TRANS_ACTIONS_WI()
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

void RubyCodeGen::writeExports()
{
	if ( exportList.length() > 0 ) {
		for ( ExportList::Iter ex = exportList; ex.lte(); ex++ ) {
			STATIC_VAR( ALPH_TYPE(), DATA_PREFIX() + "ex_" + ex->name ) 
					<< " = " << KEY(ex->key) << "\n";
		}
		out << "\n";
	}
}


void RubyCodeGen::writeData()
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

	if ( redFsm->startState != 0 )
		STATIC_VAR( "int", START() ) << " = " << START_STATE_ID() << ";\n";

	if ( writeFirstFinal )
		STATIC_VAR( "int" , FIRST_FINAL() ) << " = " << FIRST_FINAL_STATE() << ";\n";

	if ( writeErr )
		STATIC_VAR( "int", ERROR() ) << " = " << ERROR_STATE() << ";\n";
	
	out << "\n";

	if ( entryPointNames.length() > 0 ) {
		for ( EntryNameVect::Iter en = entryPointNames; en.lte(); en++ ) {
			STATIC_VAR( "int", DATA_PREFIX() + "en_" + *en ) << 
					" = " << entryPointIds[en.pos()] << ";\n";
		}
		out << "\n";
	}
}

std::ostream &RubyCodeGen::START_ARRAY_LINE()
{
	out << "\t";
	return out;
}

std::ostream &RubyCodeGen::ARRAY_ITEM( string item, int count, bool last )
{
	out << item;
	if ( !last )
	{
		out << ", ";
		if ( count % IALL == 0 )
		{
			END_ARRAY_LINE();
			START_ARRAY_LINE();
		}
	}
	return out;
}

std::ostream &RubyCodeGen::END_ARRAY_LINE()
{
	out << "\n";
	return out;
}


unsigned int RubyCodeGen::arrayTypeSize( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	HostType *arrayType = keyOps->typeSubsumes( maxValLL );
	assert( arrayType != 0 );
	return arrayType->size;
}

string RubyCodeGen::ARRAY_TYPE( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	HostType *arrayType = keyOps->typeSubsumes( maxValLL );
	assert( arrayType != 0 );

	string ret = arrayType->data1;
	if ( arrayType->data2 != 0 ) {
		ret += " ";
		ret += arrayType->data2;
	}
	return ret;
}


/* Write out the fsm name. */
string RubyCodeGen::FSM_NAME()
{
	return fsmName;
}

/* Emit the offset of the start state as a decimal integer. */
string RubyCodeGen::START_STATE_ID()
{
	ostringstream ret;
	ret << redFsm->startState->id;
	return ret.str();
};

/* Write out the array of actions. */
std::ostream &RubyCodeGen::ACTIONS_ARRAY()
{
	START_ARRAY_LINE();
	int totalActions = 0;
	ARRAY_ITEM( INT(0), ++totalActions, false );
	for ( ActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		/* Write out the length, which will never be the last character. */
		ARRAY_ITEM( INT(act->key.length()), ++totalActions, false );

		for ( ActionTable::Iter item = act->key; item.lte(); item++ ) {
			ARRAY_ITEM( INT(item->value->actionId), ++totalActions, (act.last() && item.last()) );
		}
	}
	END_ARRAY_LINE();
	return out;
}


string RubyCodeGen::CS()
{
	ostringstream ret;
	if ( curStateExpr != 0 ) { 
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, curStateExpr, 0, false );
		ret << ")";
	}
	else {
		/* Expression for retrieving the key, use simple dereference. */
		ret << ACCESS() << "cs";
	}
	return ret.str();
}

string RubyCodeGen::ACCESS()
{
	ostringstream ret;
	if ( accessExpr != 0 )
		INLINE_LIST( ret, accessExpr, 0, false );
	return ret.str();
}

string RubyCodeGen::GET_WIDE_KEY()
{
	if ( redFsm->anyConditions() ) 
		return "_widec";
	else
		return GET_KEY();
}

string RubyCodeGen::GET_WIDE_KEY( RedStateAp *state )
{
	if ( state->stateCondList.length() > 0 )
		return "_widec";
	else
		return GET_KEY();
}

/* Write out level number of tabs. Makes the nested binary search nice
 * looking. */
string RubyCodeGen::TABS( int level )
{
	string result;
	while ( level-- > 0 )
		result += "\t";
	return result;
}

string RubyCodeGen::KEY( Key key )
{
	ostringstream ret;
	if ( keyOps->isSigned || !hostLang->explicitUnsigned )
		ret << key.getVal();
	else
		ret << (unsigned long) key.getVal();
	return ret.str();
}

string RubyCodeGen::INT( int i )
{
	ostringstream ret;
	ret << i;
	return ret.str();
}

void RubyCodeGen::LM_SWITCH( ostream &ret, InlineItem *item, 
		int targState, int inFinish )
{
	ret << 
		"	case " << ACT() << "\n";

	/* If the switch handles error then we also forced the error state. It
	 * will exist. */
	if ( item->handlesError ) {
		ret << "	when 0: " << TOKEND() << " = " << TOKSTART() << "; ";
		GOTO( ret, redFsm->errState->id, inFinish );
		ret << "\n";
	}

	for ( InlineList::Iter lma = *item->children; lma.lte(); lma++ ) {
		/* Write the case label, the action and the case break. */
		ret << "	when " << lma->lmId << ":\n";

		/* Write the block and close it off. */
		ret << "	begin";
		INLINE_LIST( ret, lma->children, targState, inFinish );
		ret << "end\n";
	}

	ret << "end \n\t";
}

void RubyCodeGen::SET_ACT( ostream &ret, InlineItem *item )
{
	ret << ACT() << " = " << item->lmId << ";";
}

void RubyCodeGen::SET_TOKEND( ostream &ret, InlineItem *item )
{
	/* The tokend action sets tokend. */
	ret << TOKEND() << " = " << P();
	if ( item->offset != 0 ) 
		out << "+" << item->offset;
	out << ";";
}

void RubyCodeGen::GET_TOKEND( ostream &ret, InlineItem *item )
{
	ret << TOKEND();
}

void RubyCodeGen::INIT_TOKSTART( ostream &ret, InlineItem *item )
{
	ret << TOKSTART() << " = " << NULL_ITEM() << ";";
}

void RubyCodeGen::INIT_ACT( ostream &ret, InlineItem *item )
{
	ret << ACT() << " = 0;";
}

void RubyCodeGen::SET_TOKSTART( ostream &ret, InlineItem *item )
{
	ret << TOKSTART() << " = " << P() << ";";
}

void RubyCodeGen::SUB_ACTION( ostream &ret, InlineItem *item, 
		int targState, bool inFinish )
{
	if ( item->children->length() > 0 ) {
		/* Write the block and close it off. */
		ret << " begin ";
		INLINE_LIST( ret, item->children, targState, inFinish );
		ret << " end\n";
	}
}

void RubyCodeGen::CONDITION( ostream &ret, Action *condition )
{
	ret << "\n";
	lineDirective( ret, sourceFileName, condition->loc.line );
	INLINE_LIST( ret, condition->inlineList, 0, false );
}

string RubyCodeGen::ERROR_STATE()
{
	ostringstream ret;
	if ( redFsm->errState != 0 )
		ret << redFsm->errState->id;
	else
		ret << "-1";
	return ret.str();
}

string RubyCodeGen::FIRST_FINAL_STATE()
{
	ostringstream ret;
	if ( redFsm->firstFinState != 0 )
		ret << redFsm->firstFinState->id;
	else
		ret << redFsm->nextStateId;
	return ret.str();
}

void RubyCodeGen::finishRagelDef()
{
	/* The frontend will do this for us, but it may be a good idea to force it
	 * if the intermediate file is edited. */
	redFsm->sortByStateId();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* Maybe do flat expand, otherwise choose single. */
	redFsm->chooseSingle();

	/* If any errors have occured in the input file then don't write anything. */
	if ( gblErrorCount > 0 )
		return;
	
	/* Anlayze Machine will find the final action reference counts, among
	 * other things. We will use these in reporting the usage
	 * of fsm directives in action code. */
	analyzeMachine();

	/* Determine if we should use indicies. */
	calcIndexSize();
}

ostream &RubyCodeGen::source_warning( const InputLoc &loc )
{
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": warning: ";
	return cerr;
}

ostream &RubyCodeGen::source_error( const InputLoc &loc )
{
	gblErrorCount += 1;
	assert( sourceFileName != 0 );
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": ";
	return cerr;
}


