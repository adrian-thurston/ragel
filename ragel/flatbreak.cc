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

#include "flatbreak.h"

void FlatBreak::LOCATE_TRANS()
{
	if ( redFsm->classMap == 0 ) {
		out <<
			"	" << trans << " = " << CAST(UINT()) << ARR_REF( indexDefaults ) << "[" << vCS() << "]" << ";\n";
	}
	else {
		long lowKey = redFsm->lowKey.getVal();
		long highKey = redFsm->highKey.getVal();

		bool limitLow = keyOps->eq( lowKey, keyOps->minKey );
		bool limitHigh = keyOps->eq( highKey, keyOps->maxKey );

		out <<
			"	" << keys << " = " << OFFSET( ARR_REF( transKeys ), "(" + vCS() + "<<1)" ) << ";\n"
			"	" << inds << " = " << OFFSET( ARR_REF( indices ),
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

			out << " ) {\n";
		}

		out <<
			"       " << ic << " = " << CAST("int") << ARR_REF( charClass ) << "[" << CAST("int") << GET_KEY() <<
							" - " << lowKey << "];\n"
			"		if ( " << ic << " <= " << CAST("int") << DEREF( ARR_REF( transKeys ), keys.ref() + "+1" ) << " && " <<
						"" << ic << " >= " << CAST("int") << DEREF( ARR_REF( transKeys ), keys.ref() + "" ) << " )\n"
			"			" << trans << " = " << CAST(UINT()) << DEREF( ARR_REF( indices ),
								inds.ref() + " + " + CAST("int") + "( " + ic.ref() + " - " + CAST("int") +
								DEREF( ARR_REF( transKeys ), keys.ref() + "" ) + " ) " ) << "; \n"
			"		else\n"
			"			" << trans << " = " << CAST(UINT()) << ARR_REF( indexDefaults ) <<
								"[" << vCS() << "]" << ";\n";

		if ( !limitLow || !limitHigh ) {
			out <<
				"	}\n"
				"	else {\n"
				"		" << trans << " = " << CAST(UINT()) << ARR_REF( indexDefaults ) << "[" << vCS() << "]" << ";\n"
				"	}\n"
				"\n";
		}
	}


}

void FlatBreak::LOCATE_COND()
{
	if ( red->condSpaceList.length() > 0 ) {
		out <<
			"	" << cond << " = " << CAST( UINT() ) << ARR_REF( transOffsets ) << "[" << trans << "];\n"
			"\n";

		out <<
			"	" << cpc << " = 0;\n";

		out <<
			"	switch ( " << ARR_REF( transCondSpaces ) << "[" << trans << "] ) {\n"
			"\n";

		for ( CondSpaceList::Iter csi = red->condSpaceList; csi.lte(); csi++ ) {
			GenCondSpace *condSpace = csi;
			if ( condSpace->numTransRefs > 0 ) {
				out << "	" << CASE( STR(condSpace->condSpaceId) ) << " {\n";
				for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
					out << "if ( ";
					CONDITION( out, *csi );
					Size condValOffset = (1 << csi.pos());
					out << " ) " << cpc << " += " << condValOffset << ";\n";
				}

				out << 
					"	" << CEND() << "\n}\n";
			}
		}

		out << 
			"	}\n"
			"	" << cond << " += " << CAST( UINT() ) << "" << cpc << ";\n";
	}
}
