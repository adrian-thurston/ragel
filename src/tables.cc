#include "tables.h"

void TablesGoto::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out <<
		"	{\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps;\n";

	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() || red->condSpaceList.length() > 0 )
		out << "	int _cpc;\n";

	if ( redFsm->anyRegNbreak() )
		out << "	int _nbreak;\n";

	VARS();

	out <<
		"	" << ENTRY() << " {\n";

	if ( !noEnd ) {
		testEofUsed = true;
		out << 
			"	if ( " << P() << " == " << PE() << " )\n"
			"		goto _test_eof;\n";
	}

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		goto _out;\n";
	}

	out << LABEL( "_resume" ) << " {\n";

	FROM_STATE_ACTIONS();

	NFA_PUSH();

	LOCATE_TRANS();

	out << "}\n";
	out << LABEL( "_match_cond" ) << " {\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	string cond = "_cond";
	if ( red->condSpaceList.length() == 0 )
		cond = "_trans";

	out <<
		"	" << vCS() << " = " << CAST("int") << ARR_REF( condTargs ) << "[" << cond << "];\n\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << ARR_REF( condActions ) << "[" << cond << "] == 0 )\n"
			"		goto _again;\n"
			"\n";

		if ( redFsm->anyRegNbreak() )
			out << "	_nbreak = 0;\n";

		REG_ACTIONS( cond );

		if ( redFsm->anyRegNbreak() ) {
			out <<
				"	if ( _nbreak == 1 )\n"
				"		goto _out;\n";
			outLabelUsed = true;
		}

		out << "\n";
	}

	if ( redFsm->anyRegActions() || redFsm->anyActionGotos() || 
			redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "}\n" << LABEL( "_again" ) << " {\n";

	TO_STATE_ACTIONS();

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		goto _out;\n";
	}

	if ( !noEnd ) {
		out << 
			"	" << P() << " += 1;\n"
			"	if ( " << P() << " != " << PE() << " )\n"
			"		goto _resume;\n";
	}
	else {
		out << 
			"	" << P() << " += 1;\n"
			"	goto _resume;\n";
	}

	if ( testEofUsed )
		out << "}\n" << LABEL( "_test_eof" ) << " { {}\n";

	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out << 
			"	if ( " << P() << " == " << vEOF() << " )\n"
			"	{\n";

		NFA_PUSH();

		out <<
			"	if ( " << ARR_REF( eofCondSpaces ) << "[" << vCS() << "] != -1 ) {\n"
			"		_ckeys = " << OFFSET( ARR_REF( eofCondKeys ),
						/*CAST( UINT() ) + */ ARR_REF( eofCondKeyOffs ) + "[" + vCS() + "]" ) << ";\n"
			"		_klen = " << CAST( "int" ) << ARR_REF( eofCondKeyLens ) + "[" + vCS() + "]" << ";\n"
			"		_cpc = 0;\n"
		;

		if ( red->condSpaceList.length() > 0 )
			COND_EXEC( ARR_REF( eofCondSpaces ) + "[" + vCS() + "]" );

		COND_BIN_SEARCH( eofCondKeys, "goto _ok;", "goto _out;" );

		out << 
			"		_ok: {}\n"
			"	}\n"
		;

		outLabelUsed = true;

		EOF_ACTIONS();

		if ( redFsm->anyEofTrans() ) {
			out <<
				"	if ( " << ARR_REF( eofTrans ) << "[" << vCS() << "] > 0 ) {\n";

			EOF_TRANS();

			out <<
				"		goto _match_cond;\n"
				"	}\n";
		}

		out <<
			"	}\n"
			"\n";
	}

	if ( outLabelUsed )
		out << "}\n" << LABEL( "_out" ) << " { {}\n";

	out << "}\n";

	out << "}\n";

	NFA_POP();

	out << "	}\n";
}

void TablesVar::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;
	matchCondLabelUsed = false;

	if ( redFsm->anyNfaStates() ) {
		out <<
			"{\n"
			"	" << UINT() << " _nfa_cont = 1;\n"
			"	" << UINT() << " _nfa_repeat = 1;\n"
			"	while ( _nfa_cont != 0 )\n";
	}

	out <<
		"	{\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps;\n";

	out <<
		"	" << UINT() << " _trans = 0;\n"
		"	" << UINT() << " _have = 0;\n"
		"	" << UINT() << " _cont = 1;\n";

	VARS();

	out <<
		"	while ( _cont == 1 ) {\n"
		"\n";

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		_cont = 0;\n";
	}

	out << 
		"_have = 0;\n";

	if ( !noEnd ) {
		out << 
			"	if ( " << P() << " == " << PE() << " ) {\n";

		if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
			out << 
				"	if ( " << P() << " == " << vEOF() << " )\n"
				"	{\n";

			NFA_PUSH();

			out << UINT() << " _eofcont = 0;\n";

			out <<
				"	if ( " << ARR_REF( eofCondSpaces ) << "[" << vCS() << "] != -1 ) {\n"
				"		_ckeys = " << OFFSET( ARR_REF( eofCondKeys ),
							/*CAST( UINT() ) + */ ARR_REF( eofCondKeyOffs ) + "[" + vCS() + "]" ) << ";\n"
				"		_klen = " << CAST( "int" ) << ARR_REF( eofCondKeyLens ) + "[" + vCS() + "]" << ";\n"
				"		_cpc = 0;\n"
			;

			if ( red->condSpaceList.length() > 0 )
				COND_EXEC( ARR_REF( eofCondSpaces ) + "[" + vCS() + "]" );

			out <<
				"	{\n"
				"		" << INDEX( ARR_TYPE( eofCondKeys ), "_lower" ) << ";\n"
				"		" << INDEX( ARR_TYPE( eofCondKeys ), "_mid" ) << ";\n"
				"		" << INDEX( ARR_TYPE( eofCondKeys ), "_upper" ) << ";\n"
				"		_lower = _ckeys;\n"
				"		_upper = _ckeys + _klen - 1;\n"
				"		while ( _eofcont == 0 && _lower <= _upper ) {\n"
				"			_mid = _lower + ((_upper-_lower) >> 1);\n"
				"			if ( _cpc < " << CAST( "int" ) << DEREF( ARR_REF( eofCondKeys ), "_mid" ) << " )\n"
				"				_upper = _mid - 1;\n"
				"			else if ( _cpc > " << CAST("int" ) << DEREF( ARR_REF( eofCondKeys ), "_mid" ) << " )\n"
				"				_lower = _mid + 1;\n"
				"			else {\n"
				"				_eofcont = 1;\n"
				"			}\n"
				"		}\n"
				"		if ( _eofcont == 0 ) {\n"
				"			" << vCS() << " = " << ERROR_STATE() << ";\n"
				"		}\n"
				"	}\n"
			;

			out << 
				"	}\n"
				"	else { _eofcont = 1; }\n"
			;

			out << "if ( _eofcont == 1 ) {\n";

			EOF_ACTIONS();

			out << "	}\n";

			out << "if ( _eofcont == 1 ) {\n";

			if ( redFsm->anyEofTrans() ) {
				out <<
					"	if ( " << ARR_REF( eofTrans ) << "[" << vCS() << "] > 0 ) {\n";

				EOF_TRANS();

				out <<
					"		_have = 1;\n"
					"	}\n";

				matchCondLabelUsed = true;
			}

			out << "}\n";
			out << "}\n";
		}

		out << 
			"	if ( _have == 0 )\n"
			"		_cont = 0;\n"
			"	}\n";
	}

	out << 
		"	if ( _cont == 1 ) {\n"
		"	if ( _have == 0 ) {\n";

	FROM_STATE_ACTIONS();

	NFA_PUSH();

	LOCATE_TRANS();

	out << "}\n";

	out << "if ( _cont == 1 ) {\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	string cond = "_cond";
	if ( red->condSpaceList.length() == 0 )
		cond = "_trans";

	out <<
		"	" << vCS() << " = " << CAST("int") << ARR_REF( condTargs ) << "[" << cond << "];\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << ARR_REF( condActions ) << "[" << cond << "] != 0 ) {\n";

		REG_ACTIONS( cond );

		out <<
			"	}\n";
	}

	TO_STATE_ACTIONS();

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		_cont = 0;\n";
	}

	out << 
		"	if ( _cont == 1 )\n"
		"		" << P() << " += 1;\n";

	out << "}\n";
	out << "}\n";
	out << "}\n";

	NFA_POP();

	out << "}\n";

	if ( redFsm->anyNfaStates() )
		out << "}\n";
}
