/*
 *  Copyright 2004-2014 Adrian Thurston <thurston@complang.org>
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

Flat::Flat( const CodeGenArgs &args ) 
:
	CodeGen( args ),
	actions(          "actions",             *this ),
	keys(             "trans_keys",          *this ),
	charClass(        "char_class",          *this ),
	flatIndexOffset(  "index_offsets",       *this ),
	indicies(         "indicies",            *this ),
	indexDefaults(    "index_defaults",      *this ),
	transCondSpaces(  "trans_cond_spaces",   *this ),
	transOffsets(     "trans_offsets",       *this ),
	condTargs(        "cond_targs",          *this ),
	condActions(      "cond_actions",        *this ),
	toStateActions(   "to_state_actions",    *this ),
	fromStateActions( "from_state_actions",  *this ),
	eofActions(       "eof_actions",         *this ),
	eofTrans(         "eof_trans",           *this ),
	nfaTargs(         "nfa_targs",           *this ),
	nfaOffsets(       "nfa_offsets",         *this ),
	nfaPushActions(   "nfa_push_actions",    *this ),
	nfaPopTrans(      "nfa_pop_trans",       *this )
{}

void Flat::setKeyType()
{
	keys.setType( ALPH_TYPE(), keyOps->alphType->size, keyOps->alphType->isChar );
	keys.isSigned = keyOps->isSigned;
}

void Flat::setTableState( TableArray::State state )
{
	for ( ArrayVector::Iter i = arrayVector; i.lte(); i++ ) {
		TableArray *tableArray = *i;
		tableArray->setState( state );
	}
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
			curIndOffset += ( st->high - st->low + 1 );
	}

	flatIndexOffset.finish();
}

void Flat::taCharClass()
{
	charClass.start();

	if ( redFsm->classMap != 0 ) {
		long long maxSpan = keyOps->span( redFsm->lowKey, redFsm->highKey );

		for ( long long pos = 0; pos < maxSpan; pos++ )
			charClass.value( redFsm->classMap[pos] );
	}

	charClass.finish();
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
	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	long *transPos = new long[redFsm->transSet.length()];
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];
		transPos[trans->id] = t;
	}

	eofTrans.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		long trans = 0;

		if ( st->eofTrans != 0 )
			trans = transPos[st->eofTrans->id] + 1;

		eofTrans.value( trans );
	}

	eofTrans.finish();

	delete[] transPtrs;
	delete[] transPos;
}

void Flat::taKeys()
{
	keys.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->transList ) {
			/* Emit just low key and high key. */
			keys.value( st->low );
			keys.value( st->high );
		}
		else {
			/* Emit an impossible range so the driver fails the lookup. */
			keys.value( 1 );
			keys.value( 0 );
		}
	}

	keys.finish();
}

void Flat::taIndicies()
{
	indicies.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->transList != 0 ) {
			long long span = st->high - st->low + 1;
			for ( long long pos = 0; pos < span; pos++ )
				indicies.value( st->transList[pos]->id );
		}
	}

	indicies.finish();
}

void Flat::taIndexDefaults()
{
	indexDefaults.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* The state's default index goes next. */
		if ( st->defTrans != 0 )
			indexDefaults.value( st->defTrans->id );
		else
			indexDefaults.value( 0 );
	}

	indexDefaults.finish();
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

		curOffset += trans->condFullSize();
	}

	delete[] transPtrs;

	transOffsets.finish();
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

		long fullSize = trans->condFullSize();
		RedCondPair **fullPairs = new RedCondPair*[fullSize];
		for ( long k = 0; k < fullSize; k++ )
			fullPairs[k] = trans->errCond();

		for ( int c = 0; c < trans->numConds(); c++ )
			fullPairs[trans->outCondKey( c ).getVal()] = trans->outCond( c );

		for ( int k = 0; k < fullSize; k++ ) {
			RedCondPair *cond = fullPairs[k];
			condTargs.value( cond->targ->id );
		}

		delete[] fullPairs;
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

		long fullSize = trans->condFullSize();
		RedCondPair **fullPairs = new RedCondPair*[fullSize];
		for ( long k = 0; k < fullSize; k++ )
			fullPairs[k] = trans->errCond();

		for ( int c = 0; c < trans->numConds(); c++ )
			fullPairs[trans->outCondKey( c ).getVal()] = trans->outCond( c );

		for ( int k = 0; k < fullSize; k++ ) {
			RedCondPair *cond = fullPairs[k];
			COND_ACTION( cond );
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

void Flat::taNfaTargs()
{
	nfaTargs.start();

	/* Offset of zero means no NFA targs, put a filler there. */
	nfaTargs.value( 0 );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs != 0 ) {
			nfaTargs.value( st->nfaTargs->length() );
			for ( RedNfaTargs::Iter targ = *st->nfaTargs; targ.lte(); targ++ )
				nfaTargs.value( targ->state->id );
		}
	}

	nfaTargs.finish();
}

/* These need to mirror nfa targs. */
void Flat::taNfaPushActions()
{
	nfaPushActions.start();

	nfaPushActions.value( 0 );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs != 0 ) {
			nfaPushActions.value( 0 );
			for ( RedNfaTargs::Iter targ = *st->nfaTargs; targ.lte(); targ++ )
				NFA_PUSH_ACTION( targ );
		}
	}

	nfaPushActions.finish();
}

void Flat::taNfaPopTrans()
{
	nfaPopTrans.start();

	nfaPopTrans.value( 0 );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs != 0 ) {

			nfaPopTrans.value( 0 );

			for ( RedNfaTargs::Iter targ = *st->nfaTargs; targ.lte(); targ++ )
				NFA_POP_TEST( targ );
		}
	}

	nfaPopTrans.finish();
}


void Flat::taNfaOffsets()
{
	nfaOffsets.start();

	/* Offset of zero means no NFA targs, real targs start at 1. */
	long offset = 1;

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs == 0 ) {
			nfaOffsets.value( 0 );
		}
		else {
			nfaOffsets.value( offset );
			offset += 1 + st->nfaTargs->length();
		}
	}

	nfaOffsets.finish();
}

void Flat::NFA_PUSH()
{
	if ( redFsm->anyNfaStates() ) {
		out <<
			"	if ( " << ARR_REF( nfaOffsets ) << "[" << vCS() << "] ) {\n"
			"		int alt = 0; \n"
			"		int new_recs = " << ARR_REF( nfaTargs ) << "[" << CAST("int") <<
						ARR_REF( nfaOffsets ) << "[" << vCS() << "]];\n";

		if ( nfaPrePushExpr != 0 ) {
			out << OPEN_HOST_BLOCK( nfaPrePushExpr );
			INLINE_LIST( out, nfaPrePushExpr->inlineList, 0, false, false );
			out << CLOSE_HOST_BLOCK();
		}

		out <<
			"		while ( alt < new_recs ) { \n";


		out <<
			"			nfa_bp[nfa_len].state = " << ARR_REF( nfaTargs ) << "[" << CAST("int") <<
							ARR_REF( nfaOffsets ) << "[" << vCS() << "] + 1 + alt];\n"
			"			nfa_bp[nfa_len].p = " << P() << ";\n";

		if ( redFsm->bAnyNfaPops ) {
			out <<
				"			nfa_bp[nfa_len].popTrans = " << CAST("long") <<
								ARR_REF( nfaOffsets ) << "[" << vCS() << "] + 1 + alt;\n"
				"\n"
				;
		}

		if ( redFsm->bAnyNfaPushes ) {
			out <<
				"			switch ( " << ARR_REF( nfaPushActions ) << "[" << CAST("int") <<
								ARR_REF( nfaOffsets ) << "[" << vCS() << "] + 1 + alt] ) {\n";

			/* Loop the actions. */
			for ( GenActionTableMap::Iter redAct = redFsm->actionMap;
					redAct.lte(); redAct++ )
			{
				if ( redAct->numNfaPushRefs > 0 ) {
					/* Write the entry label. */
					out << "\t " << CASE( STR( redAct->actListId+1 ) ) << " {\n";

					/* Write each action in the list of action items. */
					for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
						ACTION( out, item->value, IlOpts( 0, false, false ) );

					out << "\n\t" << CEND() << "}\n";
				}
			}

			out <<
				"			}\n";
		}


		out <<
			"			nfa_len += 1;\n"
			"			alt += 1;\n"
			"		}\n"
			"	}\n"
			;
	}
}

void Flat::NFA_POP()
{
	if ( redFsm->anyNfaStates() ) {
		out <<
			"	if ( nfa_len > 0 ) {\n"
			"		nfa_count += 1;\n"
			"		nfa_len -= 1;\n"
			"		" << P() << " = nfa_bp[nfa_len].p;\n"
			;

		if ( redFsm->bAnyNfaPops ) {
			out << 
				"		int _pop_test = 1;\n"
				"		switch ( " << ARR_REF( nfaPopTrans ) <<
							"[nfa_bp[nfa_len].popTrans] ) {\n";

			/* Loop the actions. */
			for ( GenActionTableMap::Iter redAct = redFsm->actionMap;
					redAct.lte(); redAct++ )
			{
				if ( redAct->numNfaPopTestRefs > 0 ) {
					/* Write the entry label. */
					out << "\t " << CASE( STR( redAct->actListId+1 ) ) << " {\n";

					/* Write each action in the list of action items. */
					for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
						NFA_CONDITION( out, item->value, item.last() );

					out << CEND() << "\n}\n";
				}
			}

			out <<
				"		}\n";

			out <<
				"		if ( _pop_test ) {\n"
				"			" << vCS() << " = nfa_bp[nfa_len].state;\n";

			if ( nfaPostPopExpr != 0 ) {
				out << OPEN_HOST_BLOCK( nfaPostPopExpr );
				INLINE_LIST( out, nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK();
			}

			out <<
				"			goto _resume;\n"
				"		}\n";

			if ( nfaPostPopExpr != 0 ) {
				out <<
				"			else {\n"
				"			" << OPEN_HOST_BLOCK( nfaPostPopExpr );
				INLINE_LIST( out, nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK() << "\n"
				"			};\n";
			}
		}
		else {
			out <<
				"		" << vCS() << " = nfa_bp[nfa_len].state;\n";

			if ( nfaPostPopExpr != 0 ) {
				out << OPEN_HOST_BLOCK( nfaPostPopExpr );
				INLINE_LIST( out, nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK();
			}

			out <<
				"		goto _resume;\n";
		}

		out << 
			"		goto _out;\n"
			"	}\n";
	}
}

void Flat::LOCATE_TRANS()
{
	if ( redFsm->classMap == 0 ) {
		out <<
			"	_trans = " << CAST("int") << ARR_REF( indexDefaults ) << "[" << vCS() << "]" << ";\n";
	}
	else {
		long lowKey = redFsm->lowKey.getVal();
		long highKey = redFsm->highKey.getVal();

		bool limitLow = keyOps->eq( lowKey, keyOps->minKey );
		bool limitHigh = keyOps->eq( highKey, keyOps->maxKey );

		out <<
			"	_keys = " << OFFSET( ARR_REF( keys ), "(" + vCS() + "<<1)" ) << ";\n"
			"	_inds = " << OFFSET( ARR_REF( indicies ),
					ARR_REF( flatIndexOffset ) + "[" + vCS() + "]" ) << ";\n"
			"\n";

		if ( !limitLow || !limitHigh ) {
			out << "	if ( ";

			if ( !limitHigh )
				out << GET_KEY() << " <= " << highKey;

			if ( !limitHigh && !limitLow )
				out << " && ";

			if ( !limitLow )
				out << GET_KEY() << " >= " << lowKey;

			out << " )\n	{\n";
		}

		out <<
			"       int _ic = " << CAST("int") << ARR_REF( charClass ) << "[" << GET_KEY() <<
							" - " << lowKey << "];\n"
			"		if ( _ic <= " << CAST("int") << DEREF( ARR_REF( keys ), "_keys+1" ) << " && " <<
						"_ic >= " << CAST("int") << DEREF( ARR_REF( keys ), "_keys" ) << " )\n"
			"			_trans = " << CAST("int") << DEREF( ARR_REF( indicies ),
								"_inds + " + CAST("int") + "( _ic - " + CAST("int") +
								DEREF( ARR_REF( keys ), "_keys" ) + " ) " ) << "; \n"
			"		else\n"
			"			_trans = " << CAST("int") << ARR_REF( indexDefaults ) <<
								"[" << vCS() << "]" << ";\n";

		if ( !limitLow || !limitHigh ) {
			out <<
				"	}\n"
				"	else {\n"
				"		_trans = " << CAST("int") << ARR_REF( indexDefaults ) << "[" << vCS() << "]" << ";\n"
				"	}\n"
				"\n";
		}
	}


	if ( condSpaceList.length() > 0 ) {
		out <<
			"	_cond = " << CAST( UINT() ) << ARR_REF( transOffsets ) << "[_trans];\n"
			"\n";

		out <<
			"	_cpc = 0;\n";

		out <<
			"	switch ( " << ARR_REF( transCondSpaces ) << "[_trans] ) {\n"
			"\n";

		for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
			GenCondSpace *condSpace = csi;
			if ( condSpace->numTransRefs > 0 ) {
				out << "	" << CASE( STR(condSpace->condSpaceId) ) << " {\n";
				for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
					out << TABS(2) << "if ( ";
					CONDITION( out, *csi );
					Size condValOffset = (1 << csi.pos());
					out << " ) _cpc += " << condValOffset << ";\n";
				}

				out << 
					"	" << CEND() << "}\n";
			}
		}

		out << 
			"	}\n"
			"	_cond += " << CAST( UINT() ) << "_cpc;\n";
	}
	
	out <<
		"	goto _match_cond;\n"
	;
}

void Flat::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << gotoDest << "; " << "goto _again;" << CLOSE_GEN_BLOCK();
}

void Flat::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << "; " << "goto _again;" << CLOSE_GEN_BLOCK();
}

void Flat::CURS( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_EXPR() << "_ps" << CLOSE_GEN_EXPR();
}

void Flat::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << OPEN_GEN_EXPR() << vCS() << CLOSE_GEN_EXPR();
}

void Flat::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << nextDest << ";" << CLOSE_GEN_BLOCK();
}

void Flat::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << "" << vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";" << CLOSE_GEN_BLOCK();
}

void Flat::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( prePushExpr );
		INLINE_LIST( ret, prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " << TOP() << " += 1;" << vCS() << " = " << 
			callDest << "; " << "goto _again;" << CLOSE_GEN_BLOCK();
}


void Flat::NCALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( prePushExpr );
		INLINE_LIST( ret, prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " <<
			TOP() << " += 1;" << vCS() << " = " << 
			callDest << "; " << CLOSE_GEN_BLOCK();
}


void Flat::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( prePushExpr );
		INLINE_LIST( ret, prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " <<
			TOP() << " += 1;" << vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << CLOSE_HOST_EXPR() << "; " << "goto _again;" << CLOSE_GEN_BLOCK();
}


void Flat::NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( prePushExpr );
		INLINE_LIST( ret, prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " << TOP() << " += 1;" << vCS() <<
			" = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << CLOSE_HOST_EXPR() << "; " << CLOSE_GEN_BLOCK();
}


void Flat::RET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << " -= 1;" << vCS() << " = " << STACK() << "[" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( postPopExpr );
		INLINE_LIST( ret, postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << "goto _again;" << CLOSE_GEN_BLOCK();
}

void Flat::NRET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << " -= 1;" << vCS() << " = " << STACK() << "[" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( postPopExpr );
		INLINE_LIST( ret, postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	/* FIXME: ws in front of } will cause rlhc failure. */
	ret << CLOSE_GEN_BLOCK();
}

void Flat::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << OPEN_GEN_BLOCK() << P() << " += 1; " << "goto _out; " << CLOSE_GEN_BLOCK();
}

void Flat::NBREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << OPEN_GEN_BLOCK() << P() << " += 1; " << " _nbreak = 1; " << CLOSE_GEN_BLOCK();
}
