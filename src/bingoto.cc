/*
 * Copyright 2001-2018 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "bingoto.h"

void BinGoto::tableDataPass()
{
	if ( type == Loop )
		taActions();

	taKeyOffsets();
	taSingleLens();
	taRangeLens();
	taIndexOffsets();
	taIndicies();

	taTransCondSpacesWi();
	taTransOffsetsWi();
	taTransLengthsWi();

	taTransCondSpaces();
	taTransOffsets();
	taTransLengths();

	taCondTargs();
	taCondActions();

	taToStateActions();
	taFromStateActions();
	taEofActions();
	taEofConds();
	taEofTrans();

	taKeys();
	taCondKeys();

	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();
}

void BinGoto::genAnalysis()
{
	redFsm->sortByStateId();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* Choose the singles. */
	redFsm->moveSelectTransToSingle();

	/* If any errors have occured in the input file then don't write anything. */
	if ( red->id->errorCount > 0 )
		return;

	/* Anlayze Machine will find the final action reference counts, among other
	 * things. We will use these in reporting the usage of fsm directives in
	 * action code. */
	red->analyzeMachine();

	setKeyType();

	/* Run the analysis pass over the table data. */
	setTableState( TableArray::AnalyzePass );
	tableDataPass();

	/* Switch the tables over to the code gen mode. */
	setTableState( TableArray::GeneratePass );
}

void BinGoto::writeData()
{
	if ( type == Loop ) {
		/* If there are any transtion functions then output the array. If there
		 * are none, don't bother emitting an empty array that won't be used. */
		if ( redFsm->anyActions() )
			taActions();
	}

	taKeyOffsets();
	taKeys();
	taSingleLens();
	taRangeLens();
	taIndexOffsets();

	taTransCondSpaces();
	taTransOffsets();
	taTransLengths();

	taCondKeys();

	taCondTargs();
	taCondActions();

	if ( redFsm->anyToStateActions() )
		taToStateActions();

	if ( redFsm->anyFromStateActions() )
		taFromStateActions();

	if ( redFsm->anyEofActions() )
		taEofActions();

	taEofConds();

	if ( redFsm->anyEofTrans() )
		taEofTrans();

	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();

	STATE_IDS();
}

void BinGoto::EOF_TRANS()
{
	out <<
		"_trans = " << CAST(UINT()) << ARR_REF( eofTrans ) << "[" << vCS() << "] - 1;\n";

	if ( red->condSpaceList.length() > 0 ) {
		out <<
			"_cond = " << CAST(UINT()) << ARR_REF( transOffsets ) << "[_trans];\n";
	}
}


/* --start1 */
void BinGoto::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out <<
		"	{\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps;\n";

	out <<
		"	int _klen;\n";

	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() || red->condSpaceList.length() > 0 ) {
		out <<
			"	" << INDEX( ARR_TYPE( eofCondKeys ), "_ckeys" ) << ";\n";
	}

	out << "	" << UINT() << " _trans = 0;\n";

	if ( red->condSpaceList.length() > 0 )
		out << "	" << UINT() << " _cond = 0;\n";

	out <<
		"	" << INDEX( ALPH_TYPE(), "_keys" ) << ";\n";

	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() || red->condSpaceList.length() > 0 )
		out << "	int _cpc;\n";

	if ( redFsm->anyRegNbreak() )
		out << "	int _nbreak;\n";

	if ( type == Loop ) {
		if ( redFsm->anyToStateActions() || redFsm->anyRegActions() ||
				redFsm->anyFromStateActions() )
		{
			out << 
				"	" << INDEX( ARR_TYPE( actions ), "_acts" ) << ";\n"
				"	" << UINT() << " _nacts;\n";
		}
	}

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

/* --end1 */
