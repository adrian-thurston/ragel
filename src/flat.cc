/*
 *  Copyright 2004-2014 Adrian Thurston <thurston@complang.org>
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

		curOffset += trans->numConds();
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

	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];
		transLengths.value( trans->numConds() );
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

		for ( int c = 0; c < trans->numConds(); c++ ) {
			CondKey key = trans->outCondKey( c );
			condKeys.value( key.getVal() );
		}
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

		for ( int c = 0; c < trans->numConds(); c++ ) {
			RedCondPair *cond = trans->outCond( c );
			condTargs.value( cond->targ->id );
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

		for ( int c = 0; c < trans->numConds(); c++ ) {
			RedCondPair *cond = trans->outCond( c );
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


void Flat::LOCATE_TRANS()
{
	out <<
		"	_keys = offset( " << ARR_REF( keys ) << ", " << "(" << vCS() << "<<1)" << " );\n"
		"	_inds = offset( " << ARR_REF( indicies ) << ", " << ARR_REF( flatIndexOffset ) << "[" << vCS() << "]" << " );\n"
		"\n"
		"	_slen = (int)" << ARR_REF( keySpans ) << "[" << vCS() << "];\n"
		"	if ( _slen > 0 && deref( " << ARR_REF( keys ) << ", _keys ) <=" << GET_KEY() << " && " << GET_KEY() << " <= deref( " << ARR_REF( keys ) << ", _keys+1 ) )\n"
		"		_trans = (int)deref( " << ARR_REF( indicies ) << ", _inds + (int)( " << GET_KEY() << " - deref( " << ARR_REF( keys ) << ", _keys ) ) );\n"
		"	else\n"
		"		_trans = (int)deref( " << ARR_REF( indicies ) << ", _inds + _slen );\n"
		"\n";

	out <<
		"	_ckeys = offset( " << ARR_REF( condKeys ) << ", " << ARR_REF( transOffsets ) << "[_trans] );\n"
		"	_klen = (int)" << ARR_REF( transLengths ) << "[_trans];\n"
		"	_cond = (" << UINT() << ")" << ARR_REF( transOffsets ) << "[_trans];\n"
		"\n";

	out <<
		"	_cpc = 0;\n";

	if ( condSpaceList.length() > 0 ) {
		out <<
			"	switch ( " << ARR_REF( transCondSpaces ) << "[_trans] ) {\n"
			"\n";

		for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
			GenCondSpace *condSpace = csi;
			out << "	case " << condSpace->condSpaceId << " {\n";
			for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
				out << TABS(2) << "if ( ";
				CONDITION( out, *csi );
				Size condValOffset = (1 << csi.pos());
				out << " ) _cpc += " << condValOffset << ";\n";
			}

			out << 
				"	}\n";
		}

		out << 
			"	}\n";
	}
	
	out <<
		"	{\n"
		"		index " << ARR_TYPE( condKeys ) << " _lower;\n"
		"		index " << ARR_TYPE( condKeys ) << " _mid;\n"
		"		index " << ARR_TYPE( condKeys ) << " _upper;\n"
		"		_lower = _ckeys;\n"
		"		_upper = _ckeys + _klen - 1;\n"
		"		while ( TRUE ) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( _cpc < (int)deref( " << ARR_REF( condKeys ) << ", _mid ) )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( _cpc > (int)deref( " << ARR_REF( condKeys ) << ", _mid ) )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				_cond += (" << UINT() << ")(_mid - _ckeys);\n"
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
	ret << "${" << vCS() << " = " << gotoDest << "; " << "goto _again;}$";
}

void Flat::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << "${" << vCS() << " = host( \"-\", 1 ) ={";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "}=; " << "goto _again;}$";
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
	ret << "${";

	if ( prePushExpr != 0 ) {
		ret << "host( \"-\", 1 ) ${";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
		ret << "}$ ";
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " << TOP() << " += 1;" << vCS() << " = " << 
			callDest << "; " << "goto _again;}$";
}


void Flat::NCALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << "${";

	if ( prePushExpr != 0 ) {
		ret << "host( \"-\", 1 ) ${";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
		ret << "}$ ";
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " << TOP() << " += 1;" << vCS() << " = " << 
			callDest << "; }$";
}


void Flat::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << "${";

	if ( prePushExpr != 0 ) {
		ret << "host( \"-\", 1 ) ${";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
		ret << "}$ ";
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " << TOP() << " += 1;" << vCS() <<
			" = host( \"-\", 1 ) ={";
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << "}=; " << "goto _again;}$";
}


void Flat::NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << "${";

	if ( prePushExpr != 0 ) {
		ret << "host( \"-\", 1 ) ${";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
		ret << "}$ ";
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " << TOP() << " += 1;" << vCS() <<
			" = host( \"-\", 1 ) ={";
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << "}=; }$";
}


void Flat::RET( ostream &ret, bool inFinish )
{
	ret << "${" << TOP() << " -= 1;" << vCS() << " = " << STACK() << "[" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << "host( \"-\", 1 ) ${";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}$";
	}

	ret << "goto _again;}$";
}

void Flat::NRET( ostream &ret, bool inFinish )
{
	ret << "${" << TOP() << " -= 1;" << vCS() << " = " << STACK() << "[" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << "host( \"-\", 1 ) ${";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}$";
	}

	/* FIXME: ws in front of } will cause rlhc failure. */
	ret << "}$";
}

void Flat::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "${" << P() << " += 1; " << "goto _out; }$";
}

void Flat::NBREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "${" << P() << " += 1; " << " _nbreak = 1; }$";
}
