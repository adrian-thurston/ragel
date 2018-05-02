#include "tables.h"

void Tables::CURS( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_EXPR() << "_ps" << CLOSE_GEN_EXPR();
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
			out << TABS(2) << "if ( ";
			CONDITION( out, *csi );
			Size condValOffset = (1 << csi.pos());
			out << " ) " << cpc << " += " << condValOffset << ";\n";
		}

		out << 
			"	" << CEND() << "}\n";
	}

	out << 
		"	}\n";
}

