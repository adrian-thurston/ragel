/*
 *  Copyright 2006-2007 Adrian Thurston <thurston@cs.queensu.ca>
 *            2007 Colin Fleming <colin.fleming@caverock.com>
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

#include "rlgen-java.h"
#include "javacodegen.h"
#include "redfsm.h"
#include "gendata.h"
#include <iomanip>
#include <sstream>

/* Integer array line length. */
#define IALL 8

/* Static array initialization item count 
 * (should be multiple of IALL). */
#define SAIIC 8192

using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;

void lineDirective( ostream &out, char *fileName, int line )
{
	/* Write the preprocessor line info for to the input file. */
	out << "// line " << line  << " \"";
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

void JavaTabCodeGen::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << "{" << CS() << " = " << gotoDest << "; " << 
			CTRL_FLOW() << "break _again;}";
}

void JavaTabCodeGen::GOTO_EXPR( ostream &ret, InlineItem *ilItem, bool inFinish )
{
	ret << "{" << CS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish );
	ret << "); " << CTRL_FLOW() << "break _again;}";
}

void JavaTabCodeGen::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "++] = " << CS() << "; " << CS() << " = " << 
			callDest << "; " << CTRL_FLOW() << "break _again;}";

	if ( prePushExpr != 0 )
		ret << "}";
}

void JavaTabCodeGen::CALL_EXPR( ostream &ret, InlineItem *ilItem, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "++] = " << CS() << "; " << CS() << " = (";
	INLINE_LIST( ret, ilItem->children, targState, inFinish );
	ret << "); " << CTRL_FLOW() << "break _again;}";

	if ( prePushExpr != 0 )
		ret << "}";
}

void JavaTabCodeGen::RET( ostream &ret, bool inFinish )
{
	ret << "{" << CS() << " = " << STACK() << "[--" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, postPopExpr, 0, false );
		ret << "}";
	}

	ret << CTRL_FLOW() << "break _again;}";
}

void JavaTabCodeGen::BREAK( ostream &ret, int targState )
{
	ret << CTRL_FLOW() << "break _resume;";
}

void JavaTabCodeGen::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << CS() << " = " << nextDest << ";";
}

void JavaTabCodeGen::NEXT_EXPR( ostream &ret, InlineItem *ilItem, bool inFinish )
{
	ret << CS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish );
	ret << ");";
}

void JavaTabCodeGen::EXEC( ostream &ret, InlineItem *item, int targState, int inFinish )
{
	/* The parser gives fexec two children. The double brackets are for D
	 * code. If the inline list is a single word it will get interpreted as a
	 * C-style cast by the D compiler. */
	ret << "{" << P() << " = ((";
	INLINE_LIST( ret, item->children, targState, inFinish );
	ret << "))-1;}";
}

/* Write out an inline tree structure. Walks the list and possibly calls out
 * to virtual functions than handle language specific items in the tree. */
void JavaTabCodeGen::INLINE_LIST( ostream &ret, InlineList *inlineList, 
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
			ret << P() << "--;";
			break;
		case InlineItem::Exec:
			EXEC( ret, item, targState, inFinish );
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

string JavaTabCodeGen::DATA_PREFIX()
{
	if ( dataPrefix )
		return FSM_NAME() + "_";
	return "";
}

/* Emit the alphabet data type. */
string JavaTabCodeGen::ALPH_TYPE()
{
	string ret = keyOps->alphType->data1;
	if ( keyOps->alphType->data2 != 0 ) {
		ret += " ";
		ret += + keyOps->alphType->data2;
	}
	return ret;
}

/* Emit the alphabet data type. */
string JavaTabCodeGen::WIDE_ALPH_TYPE()
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



void JavaTabCodeGen::COND_TRANSLATE()
{
	out << 
		"	_widec = " << GET_KEY() << ";\n"
		"	_keys = " << CO() << "[" << CS() << "]*2\n;"
		"	_klen = " << CL() << "[" << CS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		int _lower = _keys\n;"
		"		int _mid;\n"
		"		int _upper = _keys + (_klen<<1) - 2;\n"
		"		while (true) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1);\n"
		"			if ( " << GET_WIDE_KEY() << " < " << CK() << "[_mid] )\n"
		"				_upper = _mid - 2;\n"
		"			else if ( " << GET_WIDE_KEY() << " > " << CK() << "[_mid+1] )\n"
		"				_lower = _mid + 2;\n"
		"			else {\n"
		"				switch ( " << C() << "[" << CO() << "[" << CS() << "]"
							" + ((_mid - _keys)>>1)] ) {\n"
		;

	for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
		CondSpace *condSpace = csi;
		out << "	case " << condSpace->condSpaceId << ": {\n";
		out << TABS(2) << "_widec = " << KEY(condSpace->baseKey) << 
				" + (" << GET_KEY() << " - " << KEY(keyOps->minKey) << ");\n";

		for ( CondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			out << TABS(2) << "if ( ";
			CONDITION( out, *csi );
			Size condValOffset = ((1 << csi.pos()) * keyOps->alphSize());
			out << " ) _widec += " << condValOffset << ";\n";
		}

		out << 
			"		break;\n"
			"	}\n";
	}

	out << 
		"				}\n"
		"				break;\n"
		"			}\n"
		"		}\n"
		"	}\n"
		"\n";
}


void JavaTabCodeGen::LOCATE_TRANS()
{
	out <<
		"	_match: do {\n"
		"	_keys = " << KO() << "[" << CS() << "]" << ";\n"
		"	_trans = " << IO() << "[" << CS() << "];\n"
		"	_klen = " << SL() << "[" << CS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		int _lower = _keys;\n"
		"		int _mid;\n"
		"		int _upper = _keys + _klen - 1;\n"
		"		while (true) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( " << GET_WIDE_KEY() << " < " << K() << "[_mid] )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( " << GET_WIDE_KEY() << " > " << K() << "[_mid] )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				_trans += (_mid - _keys);\n"
		"				break _match;\n"
		"			}\n"
		"		}\n"
		"		_keys += _klen;\n"
		"		_trans += _klen;\n"
		"	}\n"
		"\n"
		"	_klen = " << RL() << "[" << CS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		int _lower = _keys;\n"
		"		int _mid;\n"
		"		int _upper = _keys + (_klen<<1) - 2;\n"
		"		while (true) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1);\n"
		"			if ( " << GET_WIDE_KEY() << " < " << K() << "[_mid] )\n"
		"				_upper = _mid - 2;\n"
		"			else if ( " << GET_WIDE_KEY() << " > " << K() << "[_mid+1] )\n"
		"				_lower = _mid + 2;\n"
		"			else {\n"
		"				_trans += ((_mid - _keys)>>1);\n"
		"				break _match;\n"
		"			}\n"
		"		}\n"
		"		_trans += _klen;\n"
		"	}\n"
		"	} while (false);\n"
		"\n";
}

/* Determine if we should use indicies or not. */
void JavaTabCodeGen::calcIndexSize()
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

int JavaTabCodeGen::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	return act;
}

int JavaTabCodeGen::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	return act;
}

int JavaTabCodeGen::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	return act;
}


int JavaTabCodeGen::TRANS_ACTION( RedTransAp *trans )
{
	/* If there are actions, emit them. Otherwise emit zero. */
	int act = 0;
	if ( trans->action != 0 )
		act = trans->action->location+1;
	return act;
}

std::ostream &JavaTabCodeGen::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &JavaTabCodeGen::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &JavaTabCodeGen::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, true );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}


std::ostream &JavaTabCodeGen::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( ActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &JavaTabCodeGen::COND_OFFSETS()
{
	int curKeyOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the key offset. */
		ARRAY_ITEM( INT(curKeyOffset), st.last() );

		/* Move the key offset ahead. */
		curKeyOffset += st->stateCondList.length();
	}
	return out;
}

std::ostream &JavaTabCodeGen::KEY_OFFSETS()
{
	int curKeyOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the key offset. */
		ARRAY_ITEM( INT(curKeyOffset), st.last() );

		/* Move the key offset ahead. */
		curKeyOffset += st->outSingle.length() + st->outRange.length()*2;
	}
	return out;
}


std::ostream &JavaTabCodeGen::INDEX_OFFSETS()
{
	int curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		ARRAY_ITEM( INT(curIndOffset), st.last() );

		/* Move the index offset ahead. */
		curIndOffset += st->outSingle.length() + st->outRange.length();
		if ( st->defTrans != 0 )
			curIndOffset += 1;
	}
	return out;
}

std::ostream &JavaTabCodeGen::COND_LENS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		ARRAY_ITEM( INT(st->stateCondList.length()), st.last() );
	}
	return out;
}


std::ostream &JavaTabCodeGen::SINGLE_LENS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		ARRAY_ITEM( INT(st->outSingle.length()), st.last() );
	}
	return out;
}

std::ostream &JavaTabCodeGen::RANGE_LENS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Emit length of range index. */
		ARRAY_ITEM( INT(st->outRange.length()), st.last() );
	}
	return out;
}

std::ostream &JavaTabCodeGen::TO_STATE_ACTIONS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		ARRAY_ITEM( INT(TO_STATE_ACTION(st)), st.last() );
	}
	return out;
}

std::ostream &JavaTabCodeGen::FROM_STATE_ACTIONS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		ARRAY_ITEM( INT(FROM_STATE_ACTION(st)), st.last() );
	}
	return out;
}

std::ostream &JavaTabCodeGen::EOF_ACTIONS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		ARRAY_ITEM( INT(EOF_ACTION(st)), st.last() );
	}
	return out;
}

std::ostream &JavaTabCodeGen::COND_KEYS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the state's transitions. */
		for ( StateCondList::Iter sc = st->stateCondList; sc.lte(); sc++ ) {
			/* Lower key. */
			ARRAY_ITEM( KEY( sc->lowKey ), false );
			ARRAY_ITEM( KEY( sc->highKey ), false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), true );
	return out;
}

std::ostream &JavaTabCodeGen::COND_SPACES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the state's transitions. */
		for ( StateCondList::Iter sc = st->stateCondList; sc.lte(); sc++ ) {
			/* Cond Space id. */
			ARRAY_ITEM( KEY( sc->condSpace->condSpaceId ), false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), true );
	return out;
}

std::ostream &JavaTabCodeGen::KEYS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			ARRAY_ITEM( KEY( stel->lowKey ), false );
		}

		/* Loop the state's transitions. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			/* Lower key. */
			ARRAY_ITEM( KEY( rtel->lowKey ), false );

			/* Upper key. */
			ARRAY_ITEM( KEY( rtel->highKey ), false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), true );
	return out;
}

std::ostream &JavaTabCodeGen::INDICIES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			ARRAY_ITEM( KEY( stel->value->id ), false );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			ARRAY_ITEM( KEY( rtel->value->id ), false );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			ARRAY_ITEM( KEY( st->defTrans->id ), false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), true );
	return out;
}

std::ostream &JavaTabCodeGen::TRANS_TARGS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			ARRAY_ITEM( KEY( trans->targ->id ), false );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			ARRAY_ITEM( KEY( trans->targ->id ), false );
		}

		/* The state's default target state. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			ARRAY_ITEM( KEY( trans->targ->id ), false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), true );
	return out;
}


std::ostream &JavaTabCodeGen::TRANS_ACTIONS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			ARRAY_ITEM( INT(TRANS_ACTION( trans )), false );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			ARRAY_ITEM( INT(TRANS_ACTION( trans )), false );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			ARRAY_ITEM( INT(TRANS_ACTION( trans )), false );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	ARRAY_ITEM( INT(0), true );
	return out;
}

std::ostream &JavaTabCodeGen::TRANS_TARGS_WI()
{
	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Write out the target state. */
		RedTransAp *trans = transPtrs[t];
		ARRAY_ITEM( INT(trans->targ->id), ( t >= redFsm->transSet.length()-1 ) );
	}
	delete[] transPtrs;
	return out;
}


std::ostream &JavaTabCodeGen::TRANS_ACTIONS_WI()
{
	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Write the function for the transition. */
		RedTransAp *trans = transPtrs[t];
		ARRAY_ITEM( INT(TRANS_ACTION( trans )), ( t >= redFsm->transSet.length()-1 ) );
	}
	delete[] transPtrs;
	return out;
}

void JavaTabCodeGen::writeExports()
{
	if ( exportList.length() > 0 ) {
		for ( ExportList::Iter ex = exportList; ex.lte(); ex++ ) {
			STATIC_VAR( ALPH_TYPE(), DATA_PREFIX() + "ex_" + ex->name ) 
					<< " = " << KEY(ex->key) << ";\n";
		}
		out << "\n";
	}
}

void JavaTabCodeGen::writeData()
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

void JavaTabCodeGen::writeExec()
{
	out <<
		"	{\n"
		"	int _klen";

	if ( redFsm->anyRegCurStateRef() )
		out << ", _ps";

	out << 
		";\n"
		"	int _trans;\n";

	if ( redFsm->anyConditions() )
		out << "	int _widec;\n";

	if ( redFsm->anyToStateActions() || redFsm->anyRegActions() || 
			redFsm->anyFromStateActions() )
	{
		out << 
			"	int _acts;\n"
			"	int _nacts;\n";
	}

	out <<
		"	int _keys;\n"
		"\n";

	if ( hasEnd )
		out << "	if ( " << P() << " != " << PE() << " ) {\n";

	if ( redFsm->errState != 0 )
		out << "	if ( " << CS() << " != " << redFsm->errState->id << " ) {\n";

	out << "	_resume: while ( true ) {\n";

	out << "	_again: do {\n";

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	_acts = " << FSA() << "[" << CS() << "]" << ";\n"
			"	_nacts = " << CAST("int") << " " << A() << "[_acts++];\n"
			"	while ( _nacts-- > 0 ) {\n"
			"		switch ( " << A() << "[_acts++] ) {\n";
			FROM_STATE_ACTION_SWITCH() <<
			"		}\n"
			"	}\n"
			"\n";
	}

	if ( redFsm->anyConditions() )
		COND_TRANSLATE();

	LOCATE_TRANS();

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << CS() << ";\n";

	if ( useIndicies )
		out << "	_trans = " << I() << "[_trans];\n";

	out <<
		"	" << CS() << " = " << TT() << "[_trans];\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << TA() << "[_trans] == 0 )\n"
			"		break _again;\n"
			"\n"
			"	_acts = " <<  TA() << "[_trans]" << ";\n"
			"	_nacts = " << CAST("int") << " " <<  A() << "[_acts++];\n"
			"	while ( _nacts-- > 0 )\n	{\n"
			"		switch ( " << A() << "[_acts++] )\n"
			"		{\n";
			ACTION_SWITCH() <<
			"		}\n"
			"	}\n"
			"\n";
	}

	/* Again loop, functions as again label. */
	out << "	} while (false);\n";

	if ( redFsm->anyToStateActions() ) {
		out <<
			"	_acts = " << TSA() << "[" << CS() << "]" << ";\n"
			"	_nacts = " << CAST("int") << " " << A() << "[_acts++];\n"
			"	while ( _nacts-- > 0 ) {\n"
			"		switch ( " << A() << "[_acts++] ) {\n";
			TO_STATE_ACTION_SWITCH() <<
			"		}\n"
			"	}\n"
			"\n";
	}

	if ( redFsm->errState != 0 ) {
		out << 
			"	if ( " << CS() << " == " << redFsm->errState->id << " )\n"
			"		break _resume;\n";
	}

	if ( hasEnd ) {
		out << 
			"	if ( ++" << P() << " == " << PE() << " )\n"
			"		break _resume;\n";
	}
	else {
		out << 
			"	" << P() << " += 1;\n";
	}

	/* Close the resume loop. */
	out << "	}\n";

	/* The if guarding on the error state. */
	if ( redFsm->errState != 0 )
		out << "	}";

	/* The if guarding on empty string. */
	if ( hasEnd )
		out << "	}\n";

	/* The execute block. */
	out << "	}\n";
}

void JavaTabCodeGen::writeEOF()
{
	if ( redFsm->anyEofActions() ) {
		out <<
			"	int _acts = " << EA() << "[" << CS() << "]" << ";\n"
			"	int _nacts = " << CAST("int") << " " << A() << "[_acts++];\n"
			"	while ( _nacts-- > 0 ) {\n"
			"		switch ( " << A() << "[_acts++] ) {\n";
			EOF_ACTION_SWITCH() <<
			"		}\n"
			"	}\n"
			"\n";
	}
}

std::ostream &JavaTabCodeGen::OPEN_ARRAY( string type, string name )
{
	array_type = type;
	array_name = name;
	item_count = 0;
	div_count = 1;

	out << 
		"private static void init_" << name << "_0( " << type << "[] r )\n"
		"{\n\t";

	return out;
}

std::ostream &JavaTabCodeGen::ARRAY_ITEM( string item, bool last )
{
	out << "r[" << item_count << "]=" << item << "; ";

	item_count += 1;
	
	if ( !last ) {
		if ( item_count % SAIIC == 0 ) {
			out << "\n}\n\n";
			out << "private static void init_" << array_name << "_" << div_count << 
					"( " << array_type << "[] r )\n{\n\t";
			div_count += 1;
		}
		else if ( item_count % IALL == 0 )
			out << "\n\t";
	}

	return out;
}

std::ostream &JavaTabCodeGen::CLOSE_ARRAY()
{
	out << "\n}\n\n";

	out << 
		"private static " << array_type << "[] create_" << array_name << "( )\n"
		"{\n"
		"	" << array_type << "[] r = new " << array_type << "[" << item_count << "];\n";

	for ( int i = 0; i < div_count; i++ )
		out << "	init_" << array_name << "_" << i << "( r );\n";

	out <<
		"	return r;\n"
		"}\n"
		"\n";

	out << 
		"private static final " << array_type << " " << array_name << 
				"[] = create_" << array_name << "();\n\n";

	return out;
}


std::ostream &JavaTabCodeGen::STATIC_VAR( string type, string name )
{
	out << "static final " << type << " " << name;
	return out;
}

string JavaTabCodeGen::ARR_OFF( string ptr, string offset )
{
	return ptr + " + " + offset;
}

string JavaTabCodeGen::CAST( string type )
{
	return "(" + type + ")";
}

string JavaTabCodeGen::NULL_ITEM()
{
	/* In java we use integers instead of pointers. */
	return "-1";
}

string JavaTabCodeGen::GET_KEY()
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
		ret << DATA() << "[" << P() << "]";
	}
	return ret.str();
}

string JavaTabCodeGen::CTRL_FLOW()
{
	return "if (true) ";
}

unsigned int JavaTabCodeGen::arrayTypeSize( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	HostType *arrayType = keyOps->typeSubsumes( maxValLL );
	assert( arrayType != 0 );
	return arrayType->size;
}

string JavaTabCodeGen::ARRAY_TYPE( unsigned long maxVal )
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
string JavaTabCodeGen::FSM_NAME()
{
	return fsmName;
}

/* Emit the offset of the start state as a decimal integer. */
string JavaTabCodeGen::START_STATE_ID()
{
	ostringstream ret;
	ret << redFsm->startState->id;
	return ret.str();
};

/* Write out the array of actions. */
std::ostream &JavaTabCodeGen::ACTIONS_ARRAY()
{
	ARRAY_ITEM( INT(0), false );
	for ( ActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		/* Write out the length, which will never be the last character. */
		ARRAY_ITEM( INT(act->key.length()), false );

		for ( ActionTable::Iter item = act->key; item.lte(); item++ )
			ARRAY_ITEM( INT(item->value->actionId), (act.last() && item.last()) );
	}
	return out;
}


string JavaTabCodeGen::ACCESS()
{
	ostringstream ret;
	if ( accessExpr != 0 )
		INLINE_LIST( ret, accessExpr, 0, false );
	return ret.str();
}

string JavaTabCodeGen::P()
{ 
	ostringstream ret;
	if ( pExpr == 0 )
		ret << "p";
	else {
		ret << "(";
		INLINE_LIST( ret, pExpr, 0, false );
		ret << ")";
	}
	return ret.str();
}

string JavaTabCodeGen::PE()
{
	ostringstream ret;
	if ( peExpr == 0 )
		ret << "pe";
	else {
		ret << "(";
		INLINE_LIST( ret, peExpr, 0, false );
		ret << ")";
	}
	return ret.str();
}

string JavaTabCodeGen::CS()
{
	ostringstream ret;
	if ( csExpr == 0 )
		ret << ACCESS() << "cs";
	else {
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, csExpr, 0, false );
		ret << ")";
	}
	return ret.str();
}

string JavaTabCodeGen::TOP()
{
	ostringstream ret;
	if ( topExpr == 0 )
		ret << ACCESS() + "top";
	else {
		ret << "(";
		INLINE_LIST( ret, topExpr, 0, false );
		ret << ")";
	}
	return ret.str();
}

string JavaTabCodeGen::STACK()
{
	ostringstream ret;
	if ( stackExpr == 0 )
		ret << ACCESS() + "stack";
	else {
		ret << "(";
		INLINE_LIST( ret, stackExpr, 0, false );
		ret << ")";
	}
	return ret.str();
}

string JavaTabCodeGen::ACT()
{
	ostringstream ret;
	if ( actExpr == 0 )
		ret << ACCESS() + "act";
	else {
		ret << "(";
		INLINE_LIST( ret, actExpr, 0, false );
		ret << ")";
	}
	return ret.str();
}

string JavaTabCodeGen::TOKSTART()
{
	ostringstream ret;
	if ( tokstartExpr == 0 )
		ret << ACCESS() + "tokstart";
	else {
		ret << "(";
		INLINE_LIST( ret, tokstartExpr, 0, false );
		ret << ")";
	}
	return ret.str();
}

string JavaTabCodeGen::TOKEND()
{
	ostringstream ret;
	if ( tokendExpr == 0 )
		ret << ACCESS() + "tokend";
	else {
		ret << "(";
		INLINE_LIST( ret, tokendExpr, 0, false );
		ret << ")";
	}
	return ret.str();
}

string JavaTabCodeGen::DATA()
{
	ostringstream ret;
	if ( dataExpr == 0 )
		ret << ACCESS() + "data";
	else {
		ret << "(";
		INLINE_LIST( ret, dataExpr, 0, false );
		ret << ")";
	}
	return ret.str();
}


string JavaTabCodeGen::GET_WIDE_KEY()
{
	if ( redFsm->anyConditions() ) 
		return "_widec";
	else
		return GET_KEY();
}

string JavaTabCodeGen::GET_WIDE_KEY( RedStateAp *state )
{
	if ( state->stateCondList.length() > 0 )
		return "_widec";
	else
		return GET_KEY();
}

/* Write out level number of tabs. Makes the nested binary search nice
 * looking. */
string JavaTabCodeGen::TABS( int level )
{
	string result;
	while ( level-- > 0 )
		result += "\t";
	return result;
}

string JavaTabCodeGen::KEY( Key key )
{
	ostringstream ret;
	if ( keyOps->isSigned || !hostLang->explicitUnsigned )
		ret << key.getVal();
	else
		ret << (unsigned long) key.getVal();
	return ret.str();
}

string JavaTabCodeGen::INT( int i )
{
	ostringstream ret;
	ret << i;
	return ret.str();
}

void JavaTabCodeGen::LM_SWITCH( ostream &ret, InlineItem *item, 
		int targState, int inFinish )
{
	ret << 
		"	switch( " << ACT() << " ) {\n";

	for ( InlineList::Iter lma = *item->children; lma.lte(); lma++ ) {
		/* Write the case label, the action and the case break. */
		ret << "	case " << lma->lmId << ":\n";

		/* Write the block and close it off. */
		ret << "	{";
		INLINE_LIST( ret, lma->children, targState, inFinish );
		ret << "}\n";

		ret << "	break;\n";
	}
	/* Default required for D code. */
	ret << 
		"	default: break;\n"
		"	}\n"
		"\t";
}

void JavaTabCodeGen::SET_ACT( ostream &ret, InlineItem *item )
{
	ret << ACT() << " = " << item->lmId << ";";
}

void JavaTabCodeGen::SET_TOKEND( ostream &ret, InlineItem *item )
{
	/* The tokend action sets tokend. */
	ret << TOKEND() << " = " << P();
	if ( item->offset != 0 ) 
		out << "+" << item->offset;
	out << ";";
}

void JavaTabCodeGen::GET_TOKEND( ostream &ret, InlineItem *item )
{
	ret << TOKEND();
}

void JavaTabCodeGen::INIT_TOKSTART( ostream &ret, InlineItem *item )
{
	ret << TOKSTART() << " = " << NULL_ITEM() << ";";
}

void JavaTabCodeGen::INIT_ACT( ostream &ret, InlineItem *item )
{
	ret << ACT() << " = 0;";
}

void JavaTabCodeGen::SET_TOKSTART( ostream &ret, InlineItem *item )
{
	ret << TOKSTART() << " = " << P() << ";";
}

void JavaTabCodeGen::SUB_ACTION( ostream &ret, InlineItem *item, 
		int targState, bool inFinish )
{
	if ( item->children->length() > 0 ) {
		/* Write the block and close it off. */
		ret << "{";
		INLINE_LIST( ret, item->children, targState, inFinish );
		ret << "}";
	}
}

void JavaTabCodeGen::ACTION( ostream &ret, Action *action, int targState, bool inFinish )
{
	/* Write the preprocessor line info for going into the source file. */
	lineDirective( ret, sourceFileName, action->loc.line );

	/* Write the block and close it off. */
	ret << "\t{";
	INLINE_LIST( ret, action->inlineList, targState, inFinish );
	ret << "}\n";
}

void JavaTabCodeGen::CONDITION( ostream &ret, Action *condition )
{
	ret << "\n";
	lineDirective( ret, sourceFileName, condition->loc.line );
	INLINE_LIST( ret, condition->inlineList, 0, false );
}

string JavaTabCodeGen::ERROR_STATE()
{
	ostringstream ret;
	if ( redFsm->errState != 0 )
		ret << redFsm->errState->id;
	else
		ret << "-1";
	return ret.str();
}

string JavaTabCodeGen::FIRST_FINAL_STATE()
{
	ostringstream ret;
	if ( redFsm->firstFinState != 0 )
		ret << redFsm->firstFinState->id;
	else
		ret << redFsm->nextStateId;
	return ret.str();
}

void JavaTabCodeGen::writeInit()
{
	out << "	{\n";

	if ( writeCS )
		out << "\t" << CS() << " = " << START() << ";\n";
	
	/* If there are any calls, then the stack top needs initialization. */
	if ( redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "\t" << TOP() << " = 0;\n";

	if ( hasLongestMatch ) {
		out << 
			"	" << TOKSTART() << " = " << NULL_ITEM() << ";\n"
			"	" << TOKEND() << " = " << NULL_ITEM() << ";\n"
			"	" << ACT() << " = 0;\n";
	}
	out << "	}\n";
}

void JavaTabCodeGen::finishRagelDef()
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

ostream &JavaTabCodeGen::source_warning( const InputLoc &loc )
{
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": warning: ";
	return cerr;
}

ostream &JavaTabCodeGen::source_error( const InputLoc &loc )
{
	gblErrorCount += 1;
	assert( sourceFileName != 0 );
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": ";
	return cerr;
}


