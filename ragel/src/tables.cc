/*
 * Copyright 2018 Adrian Thurston <thurston@colm.net>
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

#include "tables.h"

void Tables::CURS( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_EXPR() << ps << CLOSE_GEN_EXPR();
}

void Tables::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << OPEN_GEN_EXPR() << vCS() << CLOSE_GEN_EXPR();
}

void Tables::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << nextDest << ";" << CLOSE_GEN_BLOCK();
}

void Tables::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << "" << vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";" << CLOSE_GEN_BLOCK();
}

void Tables::EOF_TRANS()
{
	out <<
		"" << trans << " = " << CAST(UINT()) << ARR_REF( eofTrans ) << "[" << vCS() << "] - 1;\n";

	if ( red->condSpaceList.length() > 0 ) {
		out <<
			"" << cond << " = " << CAST(UINT()) << ARR_REF( transOffsets ) << "[" << trans << "];\n";
	}
}

void Tables::COND_EXEC( std::string expr )
{
	out <<
		"	switch ( " << expr << " ) {\n"
		"\n";

	for ( CondSpaceList::Iter csi = red->condSpaceList; csi.lte(); csi++ ) {
		GenCondSpace *condSpace = csi;
		out << "	" << CASE( STR( condSpace->condSpaceId ) ) << " {\n";
		for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			out << "if ( ";
			CONDITION( out, *csi );
			Size condValOffset = (1 << csi.pos());
			out << " ) " << cpc << " += " << condValOffset << ";\n";
		}

		out << 
			"	" << CEND() << "\n}\n";
	}

	out << 
		"	}\n";
}

