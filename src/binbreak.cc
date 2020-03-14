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

#include "binbreak.h"

void BinBreak::LOCATE_TRANS()
{
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

void BinBreak::LOCATE_COND()
{
	if ( red->condSpaceList.length() > 0 ) {
		std::stringstream success, error;

		out <<
			"	" << ckeys << " = " << OFFSET( ARR_REF( condKeys ), ARR_REF( transOffsets ) + "[" + trans.ref() + "]" ) << ";\n"
			"	" << klen << " = " << CAST( "int" ) << ARR_REF( transLengths ) << "[" << trans << "];\n"
			"	" << cond << " = " << CAST( UINT() ) << ARR_REF( transOffsets ) << "[" << trans << "];\n"
			"\n";

		out <<
			"	" << cpc << " = 0;\n";
		
		if ( red->condSpaceList.length() > 0 )
			COND_EXEC( ARR_REF( transCondSpaces ) + "[" + trans.ref() + "]" );
		
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

