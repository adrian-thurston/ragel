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


void BinGoto::COND_BIN_SEARCH( Variable &var, TableArray &keys, std::string ok, std::string error )
{
	out <<
		"	{\n"
		"		" << INDEX( ARR_TYPE( keys ), "_lower" ) << ";\n"
		"		" << INDEX( ARR_TYPE( keys ), "_mid" ) << ";\n"
		"		" << INDEX( ARR_TYPE( keys ), "_upper" ) << ";\n"
		"		_lower = " << var << ";\n"
		"		_upper = " << var << " + " << klen << " - 1;\n"
		"		while ( " << TRUE() << " ) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( " << cpc << " < " << CAST("int") << DEREF( ARR_REF( keys ), "_mid" ) << " )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( " << cpc << " > " << CAST( "int" ) << DEREF( ARR_REF( keys ), "_mid" ) << " )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				" << ok << "\n"
		"			}\n"
		"		}\n"
		"		" << vCS() << " = " << ERROR_STATE() << ";\n"
		"		" << error << "\n"
		"	}\n"
	;
}

void BinGoto::LOCATE_COND()
{
	out <<
		"	" << ckeys << " = " << OFFSET( ARR_REF( condKeys ), ARR_REF( transOffsets ) + "[" + string(trans) + "]" ) << ";\n"
		"	" << klen << " = " << CAST( "int" ) << ARR_REF( transLengths ) << "[" << trans << "];\n"
		"	" << cond << " = " << CAST( UINT() ) << ARR_REF( transOffsets ) << "[" << trans << "];\n"
		"\n";

	out <<
		"	" << cpc << " = 0;\n";
	
	if ( red->condSpaceList.length() > 0 )
		COND_EXEC( ARR_REF( transCondSpaces ) + "[" + string(trans) + "]" );
	
	COND_BIN_SEARCH(
			ckeys, condKeys,
			"" + string(cond) + " += " + CAST( UINT() ) + "(_mid - " + string(ckeys) + "); goto _match_cond;\n",
			"goto _again;\n"
	);
}

void BinGoto::LOCATE_TRANS()
{
	out <<
		"	" << keys << " = " << OFFSET( ARR_REF( transKeys ), ARR_REF( keyOffsets ) + "[" + vCS() + "]" ) << ";\n"
		"	" << trans << " = " << CAST(UINT()) << ARR_REF( indexOffsets ) << "[" << vCS() << "];\n"
		"\n"
		"	" << klen << " = " << CAST( "int" ) << ARR_REF( singleLens ) << "[" << vCS() << "];\n"
		"	if ( " << klen << " > 0 ) {\n"
		"		" << INDEX( ALPH_TYPE(), "_lower" ) << ";\n"
		"		" << INDEX( ALPH_TYPE(), "_mid" ) << ";\n"
		"		" << INDEX( ALPH_TYPE(), "_upper" ) << ";\n"
		"		_lower = " << keys << ";\n"
		"		_upper = " << keys << " + " << klen << " - 1;\n"
		"		while ( " << TRUE() << " ) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( " << GET_KEY() << " < " << DEREF( ARR_REF( transKeys ), "_mid" ) << " )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( " << GET_KEY() << " > " << DEREF( ARR_REF( transKeys ), "_mid" ) << " )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				" << trans << " += " << CAST( UINT() ) << "(_mid - " << keys << ");\n"
		"				goto _match;\n"
		"			}\n"
		"		}\n"
		"		" << keys << " += " << klen << ";\n"
		"		" << trans << " += " << CAST( UINT() ) << "" << klen << ";\n"
		"	}\n"
		"\n"
		"	" << klen << " = " << CAST("int") << ARR_REF( rangeLens ) << "[" << vCS() << "];\n"
		"	if ( " << klen << " > 0 ) {\n"
		"		" << INDEX( ALPH_TYPE(), "_lower" ) << ";\n"
		"		" << INDEX( ALPH_TYPE(), "_mid" ) << ";\n"
		"		" << INDEX( ALPH_TYPE(), "_upper" ) << ";\n"
		"		_lower = " << keys << ";\n"
		"		_upper = " << keys << " + (" << klen << "<<1) - 2;\n"
		"		while ( " << TRUE() << " ) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1);\n"
		"			if ( " << GET_KEY() << " < " << DEREF( ARR_REF( transKeys ), "_mid" ) << " )\n"
		"				_upper = _mid - 2;\n"
		"			else if ( " << GET_KEY() << " > " << DEREF( ARR_REF( transKeys ), "_mid + 1" ) << " )\n"
		"				_lower = _mid + 2;\n"
		"			else {\n"
		"				" << trans << " += " << CAST( UINT() ) << "((_mid - " << keys << ")>>1);\n"
		"				goto _match;\n"
		"			}\n"
		"		}\n"
		"		" << trans << " += " << CAST( UINT() ) << "" << klen << ";\n"
		"	}\n"
		"\n";

	out << LABEL( "_match" ) << " {\n";

	if ( red->condSpaceList.length() > 0 )
		LOCATE_COND();
	else
		out << "goto _match_cond;\n";

	out << "}\n";
}

