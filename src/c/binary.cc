/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@complang.org>
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
#include "binary.h"
#include "redfsm.h"
#include "gendata.h"

#include <assert.h>

namespace C {

Binary::Binary( const CodeGenArgs &args )
:
	CodeGen( args ),
	keyOffsets(         "key_offsets",           *this ),
	singleLens(         "single_lengths",        *this ),
	rangeLens(          "range_lengths",         *this ),
	indexOffsets(       "index_offsets",         *this ),
	indicies(           "indicies",              *this ),
	transCondSpacesWi(  "trans_cond_spaces_wi",  *this ),
	transOffsetsWi(     "trans_offsets_wi",      *this ),
	transLengthsWi(     "trans_lengths_wi",      *this ),
	transCondSpaces(    "trans_cond_spaces",     *this ),
	transOffsets(       "trans_offsets",         *this ),
	transLengths(       "trans_lengths",         *this ),
	condTargs(          "cond_targs",            *this ),
	condActions(        "cond_actions",          *this ),
	toStateActions(     "to_state_actions",      *this ),
	fromStateActions(   "from_state_actions",    *this ),
	eofActions(         "eof_actions",           *this ),
	eofTrans(           "eof_trans",             *this )
{
}

void Binary::setTransPosWi()
{
	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Record the position, need this for eofTrans. */
		RedTransAp *trans = transPtrs[t];
		trans->pos = t;
	}
	delete[] transPtrs;
}

void Binary::setTransPos()
{
	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			trans->pos = totalTrans++;
		}

		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			trans->pos = totalTrans++;
		}

		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			trans->pos = totalTrans++;
		}
	}

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			trans->pos = totalTrans++;
		}
	}
}


std::ostream &Binary::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &Binary::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &Binary::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, true, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}


std::ostream &Binary::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

void Binary::taKeyOffsets()
{
	keyOffsets.start();

	int curKeyOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		keyOffsets.value( curKeyOffset );
		curKeyOffset += st->outSingle.length() + st->outRange.length()*2;
	}

	keyOffsets.finish();
}


void Binary::taSingleLens()
{
	singleLens.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		singleLens.value( st->outSingle.length() );

	singleLens.finish();
}


void Binary::taRangeLens()
{
	rangeLens.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		rangeLens.value( st->outRange.length() );

	rangeLens.finish();
}

void Binary::taIndexOffsets()
{
	indexOffsets.start();

	int curIndOffset = 0;

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		indexOffsets.value( curIndOffset );

		/* Move the index offset ahead. */
		curIndOffset += st->outSingle.length() + st->outRange.length();
		if ( st->defTrans != 0 )
			curIndOffset += 1;
	}

	indexOffsets.finish();
}

void Binary::taToStateActions()
{
	toStateActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		TO_STATE_ACTION(st);

	toStateActions.finish();
}

void Binary::taFromStateActions()
{
	fromStateActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		FROM_STATE_ACTION(st);

	fromStateActions.finish();
}

void Binary::taEofActions()
{
	eofActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		EOF_ACTION( st );

	eofActions.finish();
}

void Binary::taEofTrans()
{
	eofTrans.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		long trans = 0;
		if ( st->eofTrans != 0 ) {
			assert( st->eofTrans->pos >= 0 );
			trans = st->eofTrans->pos+1;
		}

		eofTrans.value( trans );
	}

	eofTrans.finish();
}

std::ostream &Binary::KEYS()
{
	out << '\t';
	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			out << KEY( stel->lowKey ) << ", ";
			if ( ++totalTrans % IALL == 0 )
				out << "\n\t";
		}

		/* Loop the state's transitions. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			/* Lower key. */
			out << KEY( rtel->lowKey ) << ", ";
			if ( ++totalTrans % IALL == 0 )
				out << "\n\t";

			/* Upper key. */
			out << KEY( rtel->highKey ) << ", ";
			if ( ++totalTrans % IALL == 0 )
				out << "\n\t";
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	out << 0 << "\n";
	return out;
}

void Binary::taIndicies()
{
	indicies.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ )
			indicies.value( stel->value->id );

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ )
			indicies.value( rtel->value->id );

		/* The state's default index goes next. */
		if ( st->defTrans != 0 )
			indicies.value( st->defTrans->id );
	}

	indicies.finish();
}

void Binary::taTransCondSpaces()
{
	transCondSpaces.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			if ( trans->condSpace != 0 )
				transCondSpaces.value( trans->condSpace->condSpaceId );
			else
				transCondSpaces.value( -1 );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			if ( trans->condSpace != 0 )
				transCondSpaces.value( trans->condSpace->condSpaceId );
			else
				transCondSpaces.value( -1 );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			if ( trans->condSpace != 0 )
				transCondSpaces.value( trans->condSpace->condSpaceId );
			else
				transCondSpaces.value( -1 );
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			if ( trans->condSpace != 0 )
				transCondSpaces.value( trans->condSpace->condSpaceId );
			else
				transCondSpaces.value( -1 );
		}
	}

	transCondSpaces.finish();
}

void Binary::taTransOffsets()
{
	transOffsets.start();

	int curOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			transOffsets.value( curOffset );
			curOffset += trans->outConds.length();
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			transOffsets.value( curOffset );
			curOffset += trans->outConds.length();
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			transOffsets.value( curOffset );
			curOffset += trans->outConds.length();
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			transOffsets.value( curOffset );
			curOffset += trans->outConds.length();
		}
	}

	transOffsets.finish();
}

void Binary::taTransLengths()
{
	transLengths.start();

	int totalLengths = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			transLengths.value( trans->outConds.length() );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			transLengths.value( trans->outConds.length() );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			transLengths.value( trans->outConds.length() );
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			transLengths.value( trans->outConds.length() );
		}
	}

	transLengths.finish();
}

void Binary::taTransCondSpacesWi()
{
	transCondSpacesWi.start();

	int totalSpaces = 0;
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
		/* Cond Space id. */
		if ( trans->condSpace != 0 )
			transCondSpacesWi.value( trans->condSpace->condSpaceId );
		else
			transCondSpacesWi.value( -1 );
	}

	transCondSpacesWi.finish();
}

void Binary::taTransOffsetsWi()
{
	transOffsetsWi.start();

	int curOffset = 0;
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
		transOffsetsWi.value( curOffset );

		TransApSet::Iter next = trans;
		next.increment();

		curOffset += trans->outConds.length();
	}

	transOffsetsWi.finish();
}

void Binary::taTransLengthsWi()
{
	transLengthsWi.start();

	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
		transLengthsWi.value( trans->outConds.length() );

		TransApSet::Iter next = trans;
		next.increment();
	}

	transLengthsWi.finish();
}

std::ostream &Binary::COND_KEYS()
{
	out << '\t';
	int totalKeys = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;

			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				out << cond->key.getVal() << ", ";
				if ( ++totalKeys % IALL == 0 )
					out << "\n\t";
			}
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;

			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				out << cond->key.getVal() << ", ";
				if ( ++totalKeys % IALL == 0 )
					out << "\n\t";
			}
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;

			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				out << cond->key.getVal() << ", ";
				if ( ++totalKeys % IALL == 0 )
					out << "\n\t";
			}
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;

			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				out << cond->key.getVal() << ", ";
				if ( ++totalKeys % IALL == 0 )
					out << "\n\t";
			}
		}
	}


	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	out << 0 << "\n";
	return out;
}

void Binary::taCondTargs()
{
	condTargs.start();

	int totalConds = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				RedCondAp *c = cond->value;
				condTargs.value( c->targ->id );
			}
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				RedCondAp *c = cond->value;
				condTargs.value( c->targ->id );
			}
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				RedCondAp *c = cond->value;
				condTargs.value( c->targ->id );
			}
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				RedCondAp *c = cond->value;
				condTargs.value( c->targ->id );
			}
		}
	}

	condTargs.finish();
}

void Binary::taCondActions()
{
	condActions.start();

	int totalConds = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				RedCondAp *c = cond->value;
				COND_ACTION( c );
			}
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				RedCondAp *c = cond->value;
				COND_ACTION( c );
			}
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				RedCondAp *c = cond->value;
				COND_ACTION( c );
			}
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
				RedCondAp *c = cond->value;
				COND_ACTION( c );
			}
		}
	}

	condActions.finish();
}

void Binary::LOCATE_TRANS()
{
	out <<
		"	_keys = " << ARR_OFF( K(), keyOffsets.ref() + "[" + vCS() + "]" ) << ";\n"
		"	_trans = " << IO() << "[" << vCS() << "];\n"
		"\n"
		"	_klen = " << singleLens.ref() << "[" << vCS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		const " << ALPH_TYPE() << " *_lower = _keys;\n"
		"		const " << ALPH_TYPE() << " *_mid;\n"
		"		const " << ALPH_TYPE() << " *_upper = _keys + _klen - 1;\n"
		"		while (1) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( " << GET_KEY() << " < *_mid )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( " << GET_KEY() << " > *_mid )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				_trans += " << CAST(UINT()) << "(_mid - _keys);\n"
		"				goto _match;\n"
		"			}\n"
		"		}\n"
		"		_keys += _klen;\n"
		"		_trans += _klen;\n"
		"	}\n"
		"\n"
		"	_klen = " << rangeLens.ref() << "[" << vCS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		const " << ALPH_TYPE() << " *_lower = _keys;\n"
		"		const " << ALPH_TYPE() << " *_mid;\n"
		"		const " << ALPH_TYPE() << " *_upper = _keys + (_klen<<1) - 2;\n"
		"		while (1) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1);\n"
		"			if ( " << GET_KEY() << " < _mid[0] )\n"
		"				_upper = _mid - 2;\n"
		"			else if ( " << GET_KEY() << " > _mid[1] )\n"
		"				_lower = _mid + 2;\n"
		"			else {\n"
		"				_trans += " << CAST(UINT()) << "((_mid - _keys)>>1);\n"
		"				goto _match;\n"
		"			}\n"
		"		}\n"
		"		_trans += _klen;\n"
		"	}\n"
		"\n";
}

void Binary::LOCATE_COND()
{
	out <<
		"	_ckeys = " << ARR_OFF( CK(), TO() + "[" + "_trans" + "]" ) << ";\n"
		"	_klen = " << TL() << "[" << "_trans" << "];\n"
		"	_cond = " << TO() << "[_trans];\n"
		"\n";

	out <<
		"	_cpc = 0;\n"
		"	switch ( " << TCS() << "[" << "_trans" << "] ) {\n"
		"\n";

	for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
		GenCondSpace *condSpace = csi;
		out << "	case " << condSpace->condSpaceId << ": {\n";
		for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			out << TABS(2) << "if ( ";
			CONDITION( out, *csi );
			Size condValOffset = (1 << csi.pos());
			out << " ) _cpc += " << condValOffset << ";\n";
		}

		out << 
			"		break;\n"
			"	}\n";
	}

	out << 
		"	}\n";
	
	out <<
		"	{\n"
		"		const " << " char *_lower = _ckeys;\n"
		"		const " << " char *_mid;\n"
		"		const " << " char *_upper = _ckeys + _klen - 1;\n"
		"		while (1) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( " << "_cpc" << " < *_mid )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( " << "_cpc" << " > *_mid )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				_cond += " << CAST(UINT()) << "(_mid - _ckeys);\n"
		"				goto _match_cond;\n"
		"			}\n"
		"		}\n"
		"		" << vCS() << " = " << ERROR_STATE() << ";\n"
		"		goto _again;\n"
		"	}\n"
	;
}

void Binary::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << "{" << vCS() << " = " << gotoDest << "; " << 
			CTRL_FLOW() << "goto _again;}";
}

void Binary::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << "{" << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";
}

void Binary::CURS( ostream &ret, bool inFinish )
{
	ret << "(_ps)";
}

void Binary::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << "(" << vCS() << ")";
}

void Binary::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << vCS() << " = " << nextDest << ";";
}

void Binary::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << ");";
}

void Binary::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "++] = " << vCS() << "; " << vCS() << " = " << 
			callDest << "; " << CTRL_FLOW() << "goto _again;}";

	if ( prePushExpr != 0 )
		ret << "}";
}

void Binary::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "++] = " << vCS() << "; " << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";

	if ( prePushExpr != 0 )
		ret << "}";
}

void Binary::RET( ostream &ret, bool inFinish )
{
	ret << "{" << vCS() << " = " << STACK() << "[--" << 
			TOP() << "]; ";

	if ( postPopExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}";
	}

	ret << CTRL_FLOW() <<  "goto _again;}";
}

void Binary::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "{" << P() << "++; " << CTRL_FLOW() << "goto _out; }";
}

}
