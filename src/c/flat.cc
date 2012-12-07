/*
 *  Copyright 2004-2006 Adrian Thurston <thurston@complang.org>
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
#include "flat.h"
#include "redfsm.h"
#include "gendata.h"

namespace C {

Flat::Flat( const CodeGenArgs &args ) 
:
	CodeGen( args ),
	actions(          "actions",             *this ),
	keys(             "trans_keys",          *this ),
	keySpans(         "key_spans",           *this ),
	flatIndexOffset(  "index_offsets",       *this ),
	indicies(         "indicies",            *this ),
	transCondSpaces(  "trans_cond_spaces",   *this ),
	transOffsets(     "trans_offsets",       *this ),
	transLengths(     "trans_lengths",       *this ),
	condKeys(         "cond_keys",           *this ),
	condTargs(        "cond_targs",          *this ),
	condActions(      "cond_actions",        *this ),
	toStateActions(   "to_state_actions",    *this ),
	fromStateActions( "from_state_actions",  *this ),
	eofActions(       "eof_actions",         *this ),
	eofTrans(         "eof_trans",           *this )
{}

void Flat::setTransPos()
{
	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];
		trans->pos = t;
	}
	delete[] transPtrs;
}


void Flat::taFlatIndexOffset()
{
	flatIndexOffset.start();

	int curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		flatIndexOffset.value( curIndOffset );
		
		/* Move the index offset ahead. */
		if ( st->transList != 0 )
			curIndOffset += keyOps->span( st->lowKey, st->highKey );

		if ( st->defTrans != 0 )
			curIndOffset += 1;
	}

	flatIndexOffset.finish();
}

void Flat::taKeySpans()
{
	keySpans.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		unsigned long long span = 0;
		if ( st->transList != 0 )
			span = keyOps->span( st->lowKey, st->highKey );

		keySpans.value( span );
	}

	keySpans.finish();
}

void Flat::taToStateActions()
{
	toStateActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		TO_STATE_ACTION(st);
	}

	toStateActions.finish();
}

void Flat::taFromStateActions()
{
	fromStateActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		FROM_STATE_ACTION( st );
	}

	fromStateActions.finish();
}

void Flat::taEofActions()
{
	eofActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		EOF_ACTION( st );
	}

	eofActions.finish();
}

void Flat::taEofTrans()
{
	eofTrans.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */

		long trans = 0;
		if ( st->eofTrans != 0 ) {
			assert( st->eofTrans->pos >= 0 );
			trans = st->eofTrans->pos+1;
		}

		eofTrans.value( trans );
	}

	eofTrans.finish();
}

void Flat::taKeys()
{
	keys.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Emit just low key and high key. */
		keys.value( st->lowKey.getVal() );
		keys.value( st->highKey.getVal() );
	}

	keys.finish();
}

void Flat::taIndicies()
{
	indicies.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->transList != 0 ) {
			/* Walk the singles. */
			unsigned long long span = keyOps->span( st->lowKey, st->highKey );
			for ( unsigned long long pos = 0; pos < span; pos++ )
				indicies.value( st->transList[pos]->id );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 )
			indicies.value( st->defTrans->id );

	}

	indicies.finish();
}

void Flat::taTransCondSpaces()
{
	transCondSpaces.start();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];

		if ( trans->condSpace != 0 )
			transCondSpaces.value( trans->condSpace->condSpaceId );
		else
			transCondSpaces.value( -1 );
	}
	delete[] transPtrs;

	transCondSpaces.finish();
}

void Flat::taTransOffsets()
{
	transOffsets.start();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	int curOffset = 0;
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];

		transOffsets.value( curOffset );

		curOffset += trans->outConds.length();
	}

	delete[] transPtrs;

	transOffsets.finish();
}

void Flat::taTransLengths()
{
	transLengths.start();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */

	int totalOffsets = 0;
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];
		transLengths.value( trans->outConds.length() );
	}
	delete[] transPtrs;

	transLengths.finish();
}

void Flat::taCondKeys()
{
	condKeys.start();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];

		for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ )
			condKeys.value( cond->key.getVal() );
	}
	delete[] transPtrs;

	condKeys.finish();
}

void Flat::taCondTargs()
{
	condTargs.start();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];

		for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
			RedCondAp *c = cond->value;
			condTargs.value( c->targ->id );
		}
	}
	delete[] transPtrs;

	condTargs.finish();
}

void Flat::taCondActions()
{
	condActions.start();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];

		for ( RedCondList::Iter cond = trans->outConds; cond.lte(); cond++ ) {
			RedCondAp *c = cond->value;
			COND_ACTION( c );
		}
	}
	delete[] transPtrs;

	condActions.finish();
}

/* Write out the array of actions. */
void Flat::taActions()
{
	actions.start();

	/* Add in the the empty actions array. */
	actions.value( 0 );

	for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		/* Length first. */
		actions.value( act->key.length() );

		for ( GenActionTable::Iter item = act->key; item.lte(); item++ )
			actions.value( item->value->actionId );
	}

	actions.finish();
}


void Flat::LOCATE_TRANS()
{
	out <<
		"	_keys = " << K() << " + " << "(" + vCS() + "<<1)" << ";\n"
		"	_inds = " << I() << " + " << IO() + "[" + vCS() + "]" << ";\n"
		"\n"
		"	_slen = " << SP() << "[" << vCS() << "];\n"
		"	_trans = _inds[ _slen > 0 && _keys[0] <=" << GET_KEY() << " &&\n"
		"		" << GET_KEY() << " <= _keys[1] ?\n"
		"		" << GET_KEY() << " - _keys[0] : _slen ];\n"

		"\n";

	out <<
		"	_ckeys = " << CK() << " + " << TO() + "[" + "_trans" + "]" << ";\n"
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

void Flat::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << "{" << vCS() << " = " << gotoDest << "; " << 
			CTRL_FLOW() << "goto _again;}";
}

void Flat::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << "{" << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";
}

void Flat::CURS( ostream &ret, bool inFinish )
{
	ret << "(_ps)";
}

void Flat::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << "(" << vCS() << ")";
}

void Flat::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << vCS() << " = " << nextDest << ";";
}

void Flat::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << ");";
}

void Flat::CALL( ostream &ret, int callDest, int targState, bool inFinish )
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


void Flat::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
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


void Flat::RET( ostream &ret, bool inFinish )
{
	ret << "{" << vCS() << " = " << STACK() << "[--" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}";
	}

	ret << CTRL_FLOW() << "goto _again;}";
}

void Flat::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "{" << P() << "++; " << CTRL_FLOW() << "goto _out; }";
}

}
