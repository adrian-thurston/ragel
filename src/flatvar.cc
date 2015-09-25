#include "flatvar.h"

void FlatVar::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << gotoDest << ";" << CLOSE_GEN_BLOCK();
}

void FlatVar::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << OPEN_HOST_EXPR( "-", 1 );
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";" << CLOSE_GEN_BLOCK();
}

void FlatVar::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	error() << "cannot use fcall in -B mode" << std::endl;
	exit(1);
}

void FlatVar::NCALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( prePushExpr );
		INLINE_LIST( ret, prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " <<
			vCS() << "; " << TOP() << " += 1;" << vCS() << " = " <<
			callDest << ";" << CLOSE_GEN_BLOCK();
}

void FlatVar::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	error() << "cannot use fcall in -B mode" << std::endl;
	exit(1);
}

void FlatVar::NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( prePushExpr );
		INLINE_LIST( ret, prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " <<
			vCS() << "; " << TOP() << " += 1;" << vCS() <<
			" = " << OPEN_HOST_EXPR( "-", 1 );
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";" << CLOSE_GEN_BLOCK();
}

void FlatVar::RET( ostream &ret, bool inFinish )
{
	error() << "cannot use fret in -B mode" << std::endl;
	exit(1);
}

void FlatVar::NRET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << "-= 1;" << vCS() << " = " <<
			STACK() << "[" << TOP() << "]; ";

	if ( postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( postPopExpr );
		INLINE_LIST( ret, postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << CLOSE_GEN_BLOCK();
}

void FlatVar::BREAK( ostream &ret, int targState, bool csForced )
{
	error() << "cannot use fbreak in -B mode" << std::endl;
	exit(1);
}

void FlatVar::NBREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << OPEN_GEN_BLOCK() << P() << "+= 1; _cont = 0; " << CLOSE_GEN_BLOCK();
}

void FlatVar::NFA_POP()
{
	if ( redFsm->anyNfaStates() ) {
		out <<
			"	_nfa_repeat = 1;\n"
			"	while ( _nfa_repeat ) {\n"
			"		_nfa_repeat = 0;\n"
			"	if ( nfa_len > 0 ) {\n"
			"		int _pop_test = 1;\n"
			"		nfa_count += 1;\n"
			"		nfa_len -= 1;\n"
			"		" << P() << " = nfa_bp[nfa_len].p;\n"
			;

		if ( redFsm->bAnyNfaPops ) {
			out << 
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

					out << "\n\t" << CEND() << "}\n";
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
//				"			goto _resume;\n"
				"			_nfa_cont = 1;\n"
				"			_nfa_repeat = 0;\n"
				"		}\n";

			if ( nfaPostPopExpr != 0 ) {
				out <<
				"			else {\n"
				"			" << OPEN_HOST_BLOCK( nfaPostPopExpr );
				INLINE_LIST( out, nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK() << "\n"
//				"				goto _out;\n"
				"				_nfa_cont = 0;\n"
				"				_nfa_repeat = 1;\n"
				"			};\n";
			}
			else {
				out <<
				"			else {\n"
//				"				goto _out;\n"
				"				_nfa_cont = 0;\n"
				"				_nfa_repeat = 1;\n"
				"			}\n"
				;
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
//				"		goto _resume;\n"
				"		_nfa_cont = 1;\n"
				"		_nfa_repeat = 0;\n"
				;
		}

		out << 
			"	}\n"
			"	else {\n"
			"		_nfa_cont = 0;\n"
			"		_nfa_repeat = 0;\n"
			"	}\n"
			"}\n"
			;
	}
}

void FlatVar::LOCATE_TRANS()
{
#if 0
	out <<
		"	_keys = offset( " << ARR_REF( keys ) << ", " << ARR_REF( keyOffsets ) << "[" << vCS() << "]" << " );\n"
		"	_trans = " << CAST( UINT() ) << ARR_REF( indexOffsets ) << "[" << vCS() << "];\n"
		"	_have = 0;\n"
		"\n"
		"	_klen = " << CAST("int") << ARR_REF( singleLens ) << "[" << vCS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		index " << ALPH_TYPE() << " _lower;\n"
		"		index " << ALPH_TYPE() << " _mid;\n"
		"		index " << ALPH_TYPE() << " _upper;\n"
		"		_lower = _keys;\n"
		"		_upper = _keys + _klen - 1;\n"
		"		while ( _upper >= _lower && _have == 0 ) {\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( " << GET_KEY() << " < deref( " << ARR_REF( keys ) << ", _mid ) )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( " << GET_KEY() << " > deref( " << ARR_REF( keys ) << ", _mid ) )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				_trans += " << CAST( UINT() ) << "(_mid - _keys);\n"
		"				_have = 1;\n"
		"			}\n"
		"		}\n"
		"		if ( _have == 0 ) {\n"
		"			_keys += _klen;\n"
		"			_trans += " << CAST( UINT() ) << "_klen;\n"
		"		}\n"
		"	}\n"
		"\n"
		"	if ( _have == 0 ) {\n"
		"	_klen = " << CAST( "int" ) << ARR_REF( rangeLens ) << "[" << vCS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		index " << ALPH_TYPE() << " _lower;\n"
		"		index " << ALPH_TYPE() << " _mid;\n"
		"		index " << ALPH_TYPE() << " _upper;\n"
		"		_lower = _keys;\n"
		"		_upper = _keys + (_klen<<1) - 2;\n"
		"		while ( _have == 0 && _lower <= _upper ) {\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1);\n"
		"			if ( " << GET_KEY() << " < deref( " << ARR_REF( keys ) << ", _mid ) )\n"
		"				_upper = _mid - 2;\n"
		"			else if ( " << GET_KEY() << " > deref( " << ARR_REF( keys ) << ", _mid + 1 ) )\n"
		"				_lower = _mid + 2;\n"
		"			else {\n"
		"				_trans += " << CAST( UINT() ) << "((_mid - _keys)>>1);\n"
		"				_have = 1;\n"
		"			}\n"
		"		}\n"
		"		if ( _have == 0 )\n"
		"			_trans += " << CAST( UINT() ) << "_klen;\n"
		"	}\n"
		"	}\n"
		"\n";
#endif

	if ( redFsm->classMap == 0 ) {
		out <<
			"	_trans = " << CAST( UINT() ) << ARR_REF( indexDefaults ) << "[" << vCS() << "]" << ";\n";
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
			"       int _ic = " << CAST( "int" ) << ARR_REF( charClass ) << "[" << GET_KEY() <<
							" - " << lowKey << "];\n"
			"		if ( _ic <= " << CAST( "int" ) << DEREF( ARR_REF( keys ), "_keys+1" ) << " && " <<
						"_ic >= " << CAST( "int" ) << DEREF( ARR_REF( keys ), "_keys" ) << " )\n"
			"			_trans = " << CAST( UINT() ) << DEREF( ARR_REF( indicies ),
								"_inds + " + CAST("int") + "( _ic - " + CAST("int") + DEREF( ARR_REF( keys ),
								"_keys" ) + " ) " ) << "; \n"
			"		else\n"
			"			_trans = " << CAST( UINT() ) << ARR_REF( indexDefaults ) <<
								"[" << vCS() << "]" << ";\n";

		if ( !limitLow || !limitHigh ) {
			out <<
				"	}\n"
				"	else {\n"
				"		_trans = " << CAST( UINT() ) << ARR_REF( indexDefaults ) << "[" << vCS() << "]" << ";\n"
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

		out << 
			"	}\n"
			"	_cond += " << CAST( UINT() ) << "_cpc;\n";
	}
	
//	out <<
//		"	goto _match_cond;\n"
//	;
}

void FlatVar::LOCATE_COND()
{
#if 0
	out <<
		"	_ckeys = offset( " << ARR_REF( condKeys ) << ",  " << ARR_REF( transOffsets ) << "[_trans] );\n"
		"	_klen = (int) " << ARR_REF( transLengths ) << "[_trans];\n"
		"	_cond = " << CAST( UINT() ) << ARR_REF( transOffsets ) << "[_trans];\n"
		"	_have = 0;\n"
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
		"		while ( _have == 0 && _lower <= _upper ) {\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( _cpc < (int)deref( " << ARR_REF( condKeys ) << ", _mid ) )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( _cpc > (int)deref( " << ARR_REF( condKeys ) << ", _mid ) )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				_cond += " << CAST( UINT() ) << "(_mid - _ckeys);\n"
		"				_have = 1;\n"
		"			}\n"
		"		}\n"
		"		if ( _have == 0 ) {\n"
		"			" << vCS() << " = " << ERROR_STATE() << ";\n"
		"			_cont = 0;\n"
		"		}\n"
		"	}\n"
	;
	outLabelUsed = true;
#endif
}
