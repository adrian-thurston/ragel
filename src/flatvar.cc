/*
 * Copyright 2014-2018 Adrian Thurston <thurston@colm.net>
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

#include "flatvar.h"

#include "parsedata.h"
#include "inputdata.h"


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
			"	_keys = " << OFFSET( ARR_REF( transKeys ), "(" + vCS() + "<<1)" ) << ";\n"
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
			"       int _ic = " << CAST( "int" ) << ARR_REF( charClass ) << "[" << CAST("int") << GET_KEY() <<
							" - " << lowKey << "];\n"
			"		if ( _ic <= " << CAST( "int" ) << DEREF( ARR_REF( transKeys ), "_keys+1" ) << " && " <<
						"_ic >= " << CAST( "int" ) << DEREF( ARR_REF( transKeys ), "_keys" ) << " )\n"
			"			_trans = " << CAST( UINT() ) << DEREF( ARR_REF( indicies ),
								"_inds + " + CAST("int") + "( _ic - " + CAST("int") + DEREF( ARR_REF( transKeys ),
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


	if ( red->condSpaceList.length() > 0 ) {
		out <<
			"	_cond = " << CAST( UINT() ) << ARR_REF( transOffsets ) << "[_trans];\n"
			"\n";

		out <<
			"	_cpc = 0;\n";

		out <<
			"	switch ( " << ARR_REF( transCondSpaces ) << "[_trans] ) {\n"
			"\n";

		for ( CondSpaceList::Iter csi = red->condSpaceList; csi.lte(); csi++ ) {
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

void FlatVar::VARS()
{
	ckeys.reference();
	klen.reference();
	cond.reference();
	cpc.reference();
	keys.reference();
	inds.reference();
	acts.reference();
	nacts.reference();

	if ( !noEnd && ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) ) {
		DECLARE( INDEX( ARR_TYPE( eofCondKeys ) ), ckeys );
		DECLARE( "int",  klen );
	}

	if ( red->condSpaceList.length() > 0 )
		DECLARE( UINT(), cond, " = 0" );

	if ( red->condSpaceList.length() > 0 || redFsm->anyEofTrans() || redFsm->anyEofActions() )
		DECLARE( "int", cpc );

	if ( redFsm->classMap != 0 ) {
		DECLARE( INDEX( ALPH_TYPE() ), keys );
		DECLARE( INDEX( ARR_TYPE( indicies ) ), inds );
	}

	if ( type == Loop ) {
		if ( redFsm->anyToStateActions() || redFsm->anyRegActions() 
				|| redFsm->anyFromStateActions() )
		{
			DECLARE( INDEX( ARR_TYPE( actions ) ), acts );
			DECLARE( UINT(), nacts );
		}
	}
}

