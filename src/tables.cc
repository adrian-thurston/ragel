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
		"_trans = " << CAST(UINT()) << ARR_REF( eofTrans ) << "[" << vCS() << "] - 1;\n";

	if ( red->condSpaceList.length() > 0 ) {
		out <<
			"_cond = " << CAST(UINT()) << ARR_REF( transOffsets ) << "[_trans];\n";
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
			out << " ) _cpc += " << condValOffset << ";\n";
		}

		out << 
			"	" << CEND() << "}\n";
	}

	out << 
		"	}\n";
}

void Tables::NFA_PUSH()
{
	if ( redFsm->anyNfaStates() ) {
		out <<
			"	if ( " << ARR_REF( nfaOffsets ) << "[" << vCS() << "] ) {\n"
			"		int alt = 0; \n"
			"		int new_recs = " << ARR_REF( nfaTargs ) << "[" << CAST("int") <<
						ARR_REF( nfaOffsets ) << "[" << vCS() << "]];\n";

		if ( red->nfaPrePushExpr != 0 ) {
			out << OPEN_HOST_BLOCK( red->nfaPrePushExpr );
			INLINE_LIST( out, red->nfaPrePushExpr->inlineList, 0, false, false );
			out << CLOSE_HOST_BLOCK();
		}

		out <<
			"		while ( alt < new_recs ) { \n";


		out <<
			"			nfa_bp[nfa_len].state = " << ARR_REF( nfaTargs ) << "[" << CAST("int") <<
							ARR_REF( nfaOffsets ) << "[" << vCS() << "] + 1 + alt];\n"
			"			nfa_bp[nfa_len].p = " << P() << ";\n";

		if ( redFsm->bAnyNfaPops ) {
			out <<
				"			nfa_bp[nfa_len].popTrans = " << CAST("long") <<
								ARR_REF( nfaOffsets ) << "[" << vCS() << "] + 1 + alt;\n"
				"\n"
				;
		}

		if ( redFsm->bAnyNfaPushes ) {
			out <<
				"			switch ( " << ARR_REF( nfaPushActions ) << "[" << CAST("int") <<
								ARR_REF( nfaOffsets ) << "[" << vCS() << "] + 1 + alt] ) {\n";

			/* Loop the actions. */
			for ( GenActionTableMap::Iter redAct = redFsm->actionMap;
					redAct.lte(); redAct++ )
			{
				if ( redAct->numNfaPushRefs > 0 ) {
					/* Write the entry label. */
					out << "\t " << CASE( STR( redAct->actListId+1 ) ) << " {\n";

					/* Write each action in the list of action items. */
					for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
						ACTION( out, item->value, IlOpts( 0, false, false ) );

					out << "\n\t" << CEND() << "}\n";
				}
			}

			out <<
				"			}\n";
		}


		out <<
			"			nfa_len += 1;\n"
			"			alt += 1;\n"
			"		}\n"
			"	}\n"
			;
	}
}
