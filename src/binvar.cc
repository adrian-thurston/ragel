#include "binvar.h"

void BinaryVar::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << gotoDest << ";" << CLOSE_GEN_BLOCK();
}

void BinaryVar::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = host( \"-\", 1 ) ={";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "}=;" << CLOSE_GEN_BLOCK();
}

void BinaryVar::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	error() << "cannot use fcall in -B mode" << std::endl;
	exit(1);
}

void BinaryVar::NCALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK();
		INLINE_LIST( ret, prePushExpr, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " <<
			vCS() << "; " << TOP() << " += 1;" << vCS() << " = " <<
			callDest << ";" << CLOSE_GEN_BLOCK();
}

void BinaryVar::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	error() << "cannot use fcall in -B mode" << std::endl;
	exit(1);
}

void BinaryVar::NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK();
		INLINE_LIST( ret, prePushExpr, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " <<
			vCS() << "; " << TOP() << " += 1;" << vCS() <<
			" = host( \"-\", 1 ) ={";
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << "}=;" << CLOSE_GEN_BLOCK();
}

void BinaryVar::RET( ostream &ret, bool inFinish )
{
	error() << "cannot use fcall in -B mode" << std::endl;
	exit(1);
}

void BinaryVar::NRET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << "-= 1;" << vCS() << " = " <<
			STACK() << "[" << TOP() << "]; ";

	if ( postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK();
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << CLOSE_GEN_BLOCK();
}

void BinaryVar::BREAK( ostream &ret, int targState, bool csForced )
{
	error() << "cannot use fcall in -B mode" << std::endl;
	exit(1);
	outLabelUsed = true;
	ret << "${" << P() << "+= 1; _cont = 0; }$";
}

void BinaryVar::NBREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "${" << P() << "+= 1; _cont = 0; }$";
}


void BinaryVar::LOCATE_TRANS()
{
	out <<
		"	_keys = " << OFFSET( ARR_REF( keys ), ARR_REF( keyOffsets ) + "[" + vCS() + "]" ) << ";\n"
		"	_trans = " << CAST( UINT() ) << ARR_REF( indexOffsets ) << "[" << vCS() << "];\n"
		"	_have = 0;\n"
		"\n"
		"	_klen = " << CAST( "int" ) << ARR_REF( singleLens ) << "[" << vCS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		" << INDEX( ALPH_TYPE(), "_lower" ) << ";\n"
		"		" << INDEX( ALPH_TYPE(), "_mid" ) << ";\n"
		"		" << INDEX( ALPH_TYPE(), "_upper" ) << ";\n"
		"		_lower = _keys;\n"
		"		_upper = _keys + _klen - 1;\n"
		"		while ( _upper >= _lower && _have == 0 ) {\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( " << GET_KEY() << " < " << DEREF( ARR_REF( keys ), "_mid" ) << " )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( " << GET_KEY() << " > " << DEREF( ARR_REF( keys ), "_mid" ) << " )\n"
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
		"		" << INDEX ( ALPH_TYPE(), "_lower" ) << ";\n"
		"		" << INDEX ( ALPH_TYPE(), "_mid" ) << ";\n"
		"		" << INDEX ( ALPH_TYPE(), "_upper" ) << ";\n"
		"		_lower = _keys;\n"
		"		_upper = _keys + (_klen<<1) - 2;\n"
		"		while ( _have == 0 && _lower <= _upper ) {\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1);\n"
		"			if ( " << GET_KEY() << " < " << DEREF( ARR_REF( keys ), "_mid" ) << " )\n"
		"				_upper = _mid - 2;\n"
		"			else if ( " << GET_KEY() << " > " << DEREF( ARR_REF( keys ), "_mid + 1" ) << " )\n"
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
}

void BinaryVar::LOCATE_COND()
{
	out <<
		"	_ckeys = " << OFFSET( ARR_REF( condKeys ), ARR_REF( transOffsets ) + "[_trans]" ) << ";\n"
		"	_klen = " << CAST( "int" ) << ARR_REF( transLengths ) << "[_trans];\n"
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
		"		" << INDEX( ARR_TYPE( condKeys ), "_lower" ) << ";\n"
		"		" << INDEX( ARR_TYPE( condKeys ), "_mid" ) << ";\n"
		"		" << INDEX( ARR_TYPE( condKeys ), "_upper" ) << ";\n"
		"		_lower = _ckeys;\n"
		"		_upper = _ckeys + _klen - 1;\n"
		"		while ( _have == 0 && _lower <= _upper ) {\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( _cpc < " << CAST( "int" ) << DEREF( ARR_REF( condKeys ), "_mid" ) << " )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( _cpc > " << CAST("int" ) << DEREF( ARR_REF( condKeys ), "_mid" ) << " )\n"
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
}

