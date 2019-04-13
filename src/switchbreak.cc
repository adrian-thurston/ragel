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

#include "switchbreak.h"

std::ostream &SwitchBreak::TRANS_GOTO( RedTransAp *trans, int level )
{
	out << "_trans = 1;\n";
	return out;
}

void SwitchBreak::RANGE_B_SEARCH( RedStateAp *state, int level, Key lower, Key upper, int low, int high )
{
	/* Get the mid position, staying on the lower end of the range. */
	int mid = (low + high) >> 1;
	RedTransEl *data = state->outRange.data;

	/* Determine if we need to look higher or lower. */
	bool anyLower = mid > low;
	bool anyHigher = mid < high;

	/* Determine if the keys at mid are the limits of the alphabet. */
	bool limitLow = keyOps->eq( data[mid].lowKey, lower );
	bool limitHigh = keyOps->eq( data[mid].highKey, upper );

	if ( anyLower && anyHigher ) {
		/* Can go lower and higher than mid. */
		out << TABS(level) << "if ( " << GET_KEY() << " < " << 
				KEY(data[mid].lowKey) << " ) {\n";
		RANGE_B_SEARCH( state, level+1, lower, keyOps->sub( data[mid].lowKey, 1 ), low, mid-1 );
		out << TABS(level) << "} else if ( " << GET_KEY() << " > " << 
				KEY(data[mid].highKey) << " ) {\n";
		RANGE_B_SEARCH( state, level+1, keyOps->add( data[mid].highKey, 1 ), upper, mid+1, high );
		out << TABS(level) << "} else {\n";
		TRANS_GOTO(data[mid].value, level+1) << "\n";
		out << TABS(level) << "}\n";
	}
	else if ( anyLower && !anyHigher ) {
		/* Can go lower than mid but not higher. */
		out << TABS(level) << "if ( " << GET_KEY() << " < " << 
				KEY(data[mid].lowKey) << " ) {\n";
		RANGE_B_SEARCH( state, level+1, lower, keyOps->sub( data[mid].lowKey, 1 ), low, mid-1 );

		/* if the higher is the highest in the alphabet then there is no
		 * sense testing it. */
		if ( limitHigh ) {
			out << TABS(level) << "} else {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if ( " << GET_KEY() << " <= " << 
					KEY(data[mid].highKey) << " ) {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";

			out << "else {\n";
			DEFAULT( state );
			out << "}\n";
		}
	}
	else if ( !anyLower && anyHigher ) {
		/* Can go higher than mid but not lower. */
		out << TABS(level) << "if ( " << GET_KEY() << " > " << 
				KEY(data[mid].highKey) << " ) {\n";
		RANGE_B_SEARCH( state, level+1, keyOps->add( data[mid].highKey, 1 ), upper, mid+1, high );

		/* If the lower end is the lowest in the alphabet then there is no
		 * sense testing it. */
		if ( limitLow ) {
			out << TABS(level) << "} else {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if ( " << GET_KEY() << " >= " << 
					KEY(data[mid].lowKey) << " ) {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";

			out << "else {\n";
			DEFAULT( state );
			out << "}\n";
		}
	}
	else {
		/* Cannot go higher or lower than mid. It's mid or bust. What
		 * tests to do depends on limits of alphabet. */
		if ( !limitLow && !limitHigh ) {
			out << TABS(level) << "if ( " << KEY(data[mid].lowKey) << " <= " << 
					GET_KEY() << " && " << GET_KEY() << " <= " << 
					KEY(data[mid].highKey) << " ) {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";

			out << "else {\n";
			DEFAULT( state );
			out << "}\n";
		}
		else if ( limitLow && !limitHigh ) {
			out << TABS(level) << "if ( " << GET_KEY() << " <= " << 
					KEY(data[mid].highKey) << " ) {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";

			out << "else {\n";
			DEFAULT( state );
			out << "}\n";
		}
		else if ( !limitLow && limitHigh ) {
			out << TABS(level) << "if ( " << KEY(data[mid].lowKey) << " <= " << 
					GET_KEY() << " ) {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";

			out << "else {\n";
			DEFAULT( state );
			out << "}\n";
		}
		else {
			/* Both high and low are at the limit. No tests to do. */
			out << TABS(level) << "{\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
	}
}

void SwitchBreak::SINGLE_SWITCH( RedStateAp *st )
{
	/* Load up the singles. */
	int numSingles = st->outSingle.length();
	RedTransEl *data = st->outSingle.data;

	if ( numSingles == 1 ) {
		/* If there is a single single key then write it out as an if. */
		out << "\tif ( " << GET_KEY() << " == " << 
				KEY(data[0].lowKey) << " ) {\n\t\t"; 

		/* Virtual function for writing the target of the transition. */
		TRANS_GOTO(data[0].value, 0) << "\n";
		out << "\t}\n";

		out << "else {\n";
		NOT_SINGLE( st );
		out << "}\n";
	}
	else if ( numSingles > 1 ) {
		/* Write out single keys in a switch if there is more than one. */
		out << "\tswitch( " << GET_KEY() << " ) {\n";

		/* Write out the single indicies. */
		for ( int j = 0; j < numSingles; j++ ) {
			out << "\t\t case " << KEY(data[j].lowKey) << " {\n";
			TRANS_GOTO(data[j].value, 0) << "\n";
			out << "\t}\n";
		}

		out << "default {\n";
		NOT_SINGLE( st );
		out << "}\n";
		
		/* Close off the transition switch. */
		out << "\t}\n";
	}
}

void SwitchBreak::DEFAULT( RedStateAp *st )
{
	TRANS_GOTO( st->defTrans, 1 ) << "\n";
}

void SwitchBreak::NOT_SINGLE( RedStateAp *st )
{
	if ( st->outRange.length() > 0 ) {
		RANGE_B_SEARCH( st, 1, keyOps->minKey, keyOps->maxKey,
				0, st->outRange.length() - 1 );
	}
	else {
		DEFAULT( st );
	}
}

void SwitchBreak::LOCATE_TRANS()
{
	out <<
		"	switch ( " << vCS() << " ) {\n";

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st == redFsm->errState ) {
			out << "case " << st->id << " {\n";
			out << "}\n";
		}
		else {
			/* Label the state. */
			out << "case " << st->id << " {\n";

			/* Try singles. */
			if ( st->outSingle.length() > 0 ) {
				SINGLE_SWITCH( st );
			}
			else {
				NOT_SINGLE( st );
			}

			out << "}\n";
		}
	}

	out <<
		"	}\n"
		"\n";

	out <<
		"	" << keys << " = " << OFFSET( ARR_REF( transKeys ), ARR_REF( keyOffsets ) + "[" + vCS() + "]" ) << ";\n"
		"	" << trans << " = " << CAST(UINT()) << ARR_REF( indexOffsets ) << "[" << vCS() << "];\n"
		"\n"
		"	" << klen << " = " << CAST( "int" ) << ARR_REF( singleLens ) << "[" << vCS() << "];\n"
		"	" << have << " = 0;\n"
		"	if ( " << klen << " > 0 ) {\n"
		"		" << INDEX( ALPH_TYPE(), "_lower" ) << " = " << keys << ";\n"
		"		" << INDEX( ALPH_TYPE(), "_upper" ) << " = " << keys << " + " << klen << " - 1;\n"
		"		" << INDEX( ALPH_TYPE(), "_mid" ) << ";\n"
		"		while ( " << TRUE() << " ) {\n"
		"			if ( _upper < _lower ) {\n"
		"				" << keys << " += " << klen << ";\n"
		"				" << trans << " += " << CAST( UINT() ) << "" << klen << ";\n"
		"				break;\n"
		"			}\n"
		"\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( " << GET_KEY() << " < " << DEREF( ARR_REF( transKeys ), "_mid" ) << " )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( " << GET_KEY() << " > " << DEREF( ARR_REF( transKeys ), "_mid" ) << " )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				" << have << " = 1;\n"
		"				" << trans << " += " << CAST( UINT() ) << "(_mid - " << keys << ");\n"
		"				break;\n"
		"			}\n"
		"		}\n"
		"	}\n"
		"\n"
		"	" << klen << " = " << CAST("int") << ARR_REF( rangeLens ) << "[" << vCS() << "];\n"
		"	if ( " << have << " == 0 && " << klen << " > 0 ) {\n"
		"		" << INDEX( ALPH_TYPE(), "_lower" ) << " = " << keys << ";\n"
		"		" << INDEX( ALPH_TYPE(), "_upper" ) << " = " << keys << " + (" << klen << "<<1) - 2;\n"
		"		" << INDEX( ALPH_TYPE(), "_mid" ) << ";\n"
		"		while ( " << TRUE() << " ) {\n"
		"			if ( _upper < _lower ) {\n"
		"				" << trans << " += " << CAST( UINT() ) << "" << klen << ";\n"
		"				break;\n"
		"			}\n"
		"\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1);\n"
		"			if ( " << GET_KEY() << " < " << DEREF( ARR_REF( transKeys ), "_mid" ) << " )\n"
		"				_upper = _mid - 2;\n"
		"			else if ( " << GET_KEY() << " > " << DEREF( ARR_REF( transKeys ), "_mid + 1" ) << " )\n"
		"				_lower = _mid + 2;\n"
		"			else {\n"
		"				" << trans << " += " << CAST( UINT() ) << "((_mid - " << keys << ")>>1);\n"
		"				break;\n"
		"			}\n"
		"		}\n"
		"	}\n"
		"\n";
}

void SwitchBreak::LOCATE_COND()
{
	if ( red->condSpaceList.length() > 0 ) {
		std::stringstream success, error;

		out <<
			"	" << ckeys << " = " << OFFSET( ARR_REF( condKeys ), ARR_REF( transOffsets ) + "[" + string(trans) + "]" ) << ";\n"
			"	" << klen << " = " << CAST( "int" ) << ARR_REF( transLengths ) << "[" << trans << "];\n"
			"	" << cond << " = " << CAST( UINT() ) << ARR_REF( transOffsets ) << "[" << trans << "];\n"
			"\n";

		out <<
			"	" << cpc << " = 0;\n";
		
		if ( red->condSpaceList.length() > 0 )
			COND_EXEC( ARR_REF( transCondSpaces ) + "[" + string(trans) + "]" );
		
		success <<
			cond << " += " << CAST( UINT() ) << "(_mid - " << ckeys << ");\n";

		error <<
			cond << " = " << errCondOffset << ";\n";

		out <<
			"	{\n"
			"		" << INDEX( ARR_TYPE( condKeys ), "_lower" ) << " = " << ckeys << ";\n"
			"		" << INDEX( ARR_TYPE( condKeys ), "_upper" ) << " = " << ckeys << " + " << klen << " - 1;\n"
			"		" << INDEX( ARR_TYPE( condKeys ), "_mid" ) << ";\n"
			"		while ( " << TRUE() << " ) {\n"
			"			if ( _upper < _lower ) {\n"
			"				" << error.str() << "\n"
			"				break;\n"
			"			}\n"
			"\n"
			"			_mid = _lower + ((_upper-_lower) >> 1);\n"
			"			if ( " << cpc << " < " << CAST("int") << DEREF( ARR_REF( condKeys ), "_mid" ) << " )\n"
			"				_upper = _mid - 1;\n"
			"			else if ( " << cpc << " > " << CAST( "int" ) << DEREF( ARR_REF( condKeys ), "_mid" ) << " )\n"
			"				_lower = _mid + 1;\n"
			"			else {\n"
			"				" << success.str() << "\n"
			"				break;\n"
			"			}\n"
			"		}\n"
			"	}\n"
		;
	}

	out << EMIT_LABEL( _match_cond );
}

