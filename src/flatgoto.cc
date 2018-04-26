/*
 * Copyright 2018-2018 Adrian Thurston <thurston@colm.net>
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

#include "flatgoto.h"


void FlatGoto::VARS()
{
	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out <<
			"	int _klen;\n"
			"	" << INDEX( ARR_TYPE( eofCondKeys ), "_ckeys" ) << ";\n";
	}

	out << "	" << UINT() << " _trans = 0;\n";

	if ( red->condSpaceList.length() > 0 )
		out << "	" << UINT() << " _cond = 0;\n";

	if ( redFsm->classMap != 0 ) {
		out <<
			"	" << INDEX( ALPH_TYPE(), "_keys" ) << ";\n"
			"	" << INDEX( ARR_TYPE( indicies ), "_inds" ) << ";\n";
	}

	if ( type == Loop ) {
		if ( redFsm->anyToStateActions() || redFsm->anyRegActions() ||
				redFsm->anyFromStateActions() )
		{
			out << 
				"	" << INDEX( ARR_TYPE( actions ), "_acts" ) << ";\n"
				"	" << UINT() << " _nacts;\n";
		}
	}
}

