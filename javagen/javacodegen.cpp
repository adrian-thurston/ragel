/*
 *  Copyright 2006 Adrian Thurston <thurston@cs.queensu.ca>
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

#include "javacodegen.h"
#include "javagen.h"
#include "tabcodegen.h"
#include "redfsm.h"
#include "gendata.h"

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
	ret << "{" << STACK() << "[" << TOP() << "++] = " << CS() << "; " << CS() << " = " << 
			callDest << "; " << CTRL_FLOW() << "break _again;}";
}

void JavaTabCodeGen::CALL_EXPR( ostream &ret, InlineItem *ilItem, int targState, bool inFinish )
{
	ret << "{" << STACK() << "[" << TOP() << "++] = " << CS() << "; " << CS() << " = (";
	INLINE_LIST( ret, ilItem->children, targState, inFinish );
	ret << "); " << CTRL_FLOW() << "break _again;}";
}

void JavaTabCodeGen::RET( ostream &ret, bool inFinish )
{
	ret << "{" << CS() << " = " << STACK() << "[--" << TOP() 
			<< "]; " << CTRL_FLOW() << "break _again;}";
}

void JavaTabCodeGen::BREAK( ostream &ret, int targState )
{
	ret << CTRL_FLOW() << "break _resume;";
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
		"			else if ( " << GET_WIDE_KEY() << " > " << CK() << "[_mid] )\n"
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

void JavaTabCodeGen::writeOutData()
{
	TabCodeGen::writeOutData();
	
	out <<
		"	private static byte[] unpack_byte(String packed)\n"
		"	{\n"
		"		byte[] ret = new byte[packed.length()];\n"
		"		for (int i = 0; i < packed.length(); i++)\n"
		"		{\n"
		"			int value = packed.charAt(i);\n"
		"			value--;\n"
		"			ret[i] = (byte) value;\n"
		"		}\n"
		"		return ret;\n"
		"	}\n"
		"	private static short[] unpack_short(String packed)\n"
		"	{\n"
		"		short[] ret = new short[packed.length()];\n"
		"		for (int i = 0; i < packed.length(); i++)\n"
		"		{\n"
		"			int value = packed.charAt(i);\n"
		"			value--;\n"
		"			ret[i] = (short) value;\n"
		"		}\n"
		"		return ret;\n"
		"	}\n";

	
}

void JavaTabCodeGen::writeOutExec()
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

	out << "	_resume: while ( true ) {\n";

	out << "	_again: do {\n";

	if ( redFsm->errState != 0 ) {
		out << 
			"	if ( " << CS() << " == " << redFsm->errState->id << " )\n"
			"		break _resume;\n";
	}

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	_acts = " << FSA() << "[" << CS() << "]" << ";\n"
			"	_nacts = " << CAST("int") << " " << A() << "[_acts++];\n"
			"	while ( _nacts-- > 0 ) {\n"
			"		switch ( " << A() << "[_acts++] ) {\n";
			FROM_STATE_ACTION_SWITCH();
			SWITCH_DEFAULT() <<
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
			ACTION_SWITCH();
			SWITCH_DEFAULT() <<
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
			TO_STATE_ACTION_SWITCH();
			SWITCH_DEFAULT() <<
			"		}\n"
			"	}\n"
			"\n";
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

	/* The if guarding on empty string. */
	if ( hasEnd )
		out << "	}\n";

	/* The execute block. */
	out << "	}\n";
}

void JavaTabCodeGen::writeOutEOF()
{
	if ( redFsm->anyEofActions() ) {
		out <<
			"	int _acts = " << EA() << "[" << CS() << "]" << ";\n"
			"	int _nacts = " << CAST("int") << " " << A() << "[_acts++];\n"
			"	while ( _nacts-- > 0 ) {\n"
			"		switch ( " << A() << "[_acts++] ) {\n";
			EOF_ACTION_SWITCH();
			SWITCH_DEFAULT() <<
			"		}\n"
			"	}\n"
			"\n";
	}
}

