/*
 * Copyright 2015 Adrian Thurston <thurston@colm.net>
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

#include <string.h>
#include <stdbool.h>

#include <iostream>

#include "fsmcodegen.h"

void Compiler::writeCommitStub()
{
	*outStream <<
		"void " << objectName << "_commit_reduce_forward( program_t *prg, tree_t **root,\n"
		"		struct pda_run *pda_run, parse_tree_t *pt )\n"
		"{\n"
		"	commit_clear_parse_tree( prg, root, pda_run, pt->child );\n"
		"}\n"
		"\n"
		"long " << objectName << "_commit_union_sz( int reducer ) { return 0; }\n"
		"void " << objectName << "_init_need() {}\n"
		"int " << objectName << "_reducer_need_tok( program_t *prg, "
				"struct pda_run *pda_run, int id ) { return COLM_RN_BOTH; }\n"
		"int " << objectName << "_reducer_need_ign( program_t *prg, "
				"struct pda_run *pda_run ) { return COLM_RN_BOTH; }\n"
		"\n"
		"void " << objectName << "_read_reduce( program_t *prg, int reducer, stream_t *stream ) {}\n"
	;
}

void Compiler::findRhsRefs( bool &lhsUsed, Vector<ProdEl*> &rhsUsed,
		Vector<ProdEl*> &locUsed, Reduction *reduction, Production *production,
		const ReduceTextItemList &list )
{
	ObjectDef *objectDef = production->prodName->objectDef;

	rhsUsed.setAsNew( production->prodElList->length() );
	locUsed.setAsNew( production->prodElList->length() );

	for ( ReduceTextItemList::Iter i = list; i.lte(); i++ ) {
		if ( i->type == ReduceTextItem::LhsRef ) {
			lhsUsed = true;
		}

		if ( i->type == ReduceTextItem::RhsRef || i->type == ReduceTextItem::RhsLoc ) {
			if ( i->n > 0 ) {
				/* Numbered. */
				ProdEl *prodEl = production->prodElList->head;
				int adv = i->n - 1;
				while ( adv > 0 ) {
					prodEl = prodEl->next;
					adv -= 1;
				}

				if ( i->type == ReduceTextItem::RhsLoc )
					locUsed[i->n-1] = prodEl;
				else
					rhsUsed[i->n-1] = prodEl;
			}
			else {
				/* Named. */
				String name( i->txt.data + 1, i->txt.length() - 1 );
				ObjectField *field = objectDef->rootScope->findField( name );
				if ( field != 0 ) {
					for ( Vector<RhsVal>::Iter r = field->rhsVal; r.lte(); r++ ) {
						if ( r->prodEl->production == production ) {
							if ( i->type == ReduceTextItem::RhsLoc )
								locUsed[r->prodEl->pos] = r->prodEl;
							else
								rhsUsed[r->prodEl->pos] = r->prodEl;
						}
					}
				}
			}
		}
	}
}

void Compiler::computeNeeded( Reduction *reduction, Production *production,
		const ReduceTextItemList &list )
{
	bool lhsUsed = false;
	Vector<ProdEl*> rhsUsed;
	Vector<ProdEl*> locUsed;

	findRhsRefs( lhsUsed, rhsUsed, locUsed, reduction, production, list );

	/* Same length, can concurrently walk with one test. */
	Vector<ProdEl*>::Iter rhs = rhsUsed;
	Vector<ProdEl*>::Iter loc = locUsed;
		
	for ( ; rhs.lte(); rhs++, loc++ ) {
		ProdEl *prodEl = *rhs;
		if ( prodEl != 0 ) {
			if ( prodEl->production == production && prodEl->langEl->type == LangEl::Term )
				reduction->needData[prodEl->langEl->id] = true;
		}

		ProdEl *locEl = *loc;
		if ( locEl != 0 && locEl->production == production )
			reduction->needLoc[locEl->langEl->id] = true;
	}
}

void Compiler::loadRefs( Reduction *reduction, Production *production,
		const ReduceTextItemList &list, bool read )
{
	bool lhsUsed = false;
	Vector<ProdEl*> rhsUsed;
	Vector<ProdEl*> locUsed;

	findRhsRefs( lhsUsed, rhsUsed, locUsed, reduction, production, list );

	if ( lhsUsed ) {
		*outStream << "	lel_" << production->prodName->fullName << " *_lhs = ";

		if ( read ) {
			*outStream <<
				"&node->u." << production->prodName->fullName << ";\n";
		}
		else {
			*outStream <<
				"&((commit_reduce_union*)(lel+1))->" << production->prodName->fullName << ";\n";
		}
	}

	/*
	 * In the first pass we load using a parse tree cursor. This is for
	 * nonterms.
	 */
	bool useCursor = false;
	for ( Vector<ProdEl*>::Iter rhs = rhsUsed; rhs.lte(); rhs++ ) {
		if ( *rhs != 0 && (*rhs)->production == production &&
				(*rhs)->langEl->type != LangEl::Term )
		{
			useCursor = true;
			break;
		}
	}

	if ( useCursor ) {
		int cursorPos = 0;

		if ( read ) {
			*outStream <<
					"	struct read_reduce_node *_pt_cursor = node->child;\n";
		}
		else {
			*outStream <<
					"	struct colm_parse_tree *_pt_cursor = lel->child;\n";
		}

		/* Same length, can concurrently walk with one test. */
		Vector<ProdEl*>::Iter rhs = rhsUsed;
		Vector<ProdEl*>::Iter loc = locUsed;
		
		for ( ; rhs.lte(); rhs++, loc++ ) {
			ProdEl *prodEl = *rhs;
			if ( prodEl != 0 ) {
				while ( cursorPos < rhs.pos() ) {
					*outStream <<
						"	_pt_cursor = _pt_cursor->next;\n";
					cursorPos += 1;
				}

				if ( prodEl->production == production ) {
					if ( prodEl->langEl->type != LangEl::Term ) {
						*outStream <<
								"lel_" << prodEl->langEl->fullName << " *" "_rhs" << rhs.pos() << " = ";

						if ( read ) {
							*outStream << "&_pt_cursor->u." << prodEl->langEl->fullName << ";\n";
						}
						else {
							*outStream << "&((commit_reduce_union*)(_pt_cursor+1))->" << prodEl->langEl->fullName << ";\n";
						}
					}
				}

			}
		}
	}

	/* In the second pass we load using a tree cursor. This is for token data
	 * and locations.
	 */

	useCursor = false;
	for ( Vector<ProdEl*>::Iter rhs = rhsUsed; rhs.lte(); rhs++ ) {
		if ( *rhs != 0 && (*rhs)->production == production &&
				(*rhs)->langEl->type == LangEl::Term )
		{
			useCursor = true;
			break;
		}
	}
	for ( Vector<ProdEl*>::Iter loc = locUsed; loc.lte(); loc++ ) {
		if ( *loc != 0 ) {
			useCursor = true;
			break;
		}
	}

	if ( useCursor ) {
		int cursorPos = 0;

		if ( read ) {
			*outStream <<
					"	read_reduce_node *_tree_cursor = node->child;\n";
		}
		else {
			*outStream <<
					"	kid_t *_tree_cursor = kid->tree->child;\n";
		}

		/* Same length, can concurrently walk with one test. */
		Vector<ProdEl*>::Iter rhs = rhsUsed;
		Vector<ProdEl*>::Iter loc = locUsed;
		
		for ( ; rhs.lte(); rhs++, loc++ ) {

			ProdEl *prodEl = *rhs;

			if ( prodEl != 0 ) {
				if ( prodEl->production == production ) {
					if ( prodEl->langEl->type == LangEl::Term ) {

						while ( cursorPos < rhs.pos() ) {
							*outStream <<
								"	_tree_cursor = _tree_cursor->next;\n";
							cursorPos += 1;
						}

						*outStream << "	colm_data *_rhs" << rhs.pos() << " = ";

						if ( read ) {
							*outStream <<
									"&_tree_cursor->data;\n";
						}
						else {
							*outStream <<
									"_tree_cursor->tree->tokdata;\n";
						}
					}
				}

			}

			ProdEl *locEl = *loc;
			if ( locEl != 0 ) {
				if ( locEl->production == production ) {

					while ( cursorPos < rhs.pos() ) {
						*outStream <<
							"	_tree_cursor = _tree_cursor->next;\n";
						cursorPos += 1;
					}

					*outStream <<
						"	colm_location *_loc" << loc.pos() << " = ";

					if ( read ) {
						*outStream << "&_tree_cursor->loc;\n";
					}
					else {
						*outStream <<
							"colm_find_location( prg, _tree_cursor->tree );\n";
					}
				}
			}
		}
	}
}

void Compiler::writeRhsRef( Production *production, ReduceTextItem *i )
{
	if ( i->n > 0 ) {
		*outStream << "_rhs" << ( i->n - 1 );
	}
	else {
		ObjectDef *objectDef = production->prodName->objectDef;
		String name( i->txt.data + 1, i->txt.length() - 1 );

		/* Find the field in the rhsVal using capture field. */
		ObjectField *field = objectDef->rootScope->findField( name );
		if ( field != 0 ) {
			for ( Vector<RhsVal>::Iter r = field->rhsVal;
					r.lte(); r++ )
			{
				if ( r->prodEl->production == production )
					*outStream << "_rhs" << r->prodEl->pos;
			}
		}
	}
}

void Compiler::writeRhsLoc( Production *production, ReduceTextItem *i )
{
	if ( i->n > 0 ) {
		*outStream << "_loc" << ( i->n - 1 );
	}
	else {
		ObjectDef *objectDef = production->prodName->objectDef;
		String name( i->txt.data + 1, i->txt.length() - 1 );

		/* Find the field in the rhsVal using capture field. */
		ObjectField *field = objectDef->rootScope->findField( name );
		if ( field != 0 ) {
			for ( Vector<RhsVal>::Iter r = field->rhsVal;
					r.lte(); r++ )
			{
				if ( r->prodEl->production == production )
					*outStream << "_loc" << r->prodEl->pos;
			}
		}
	}
}

void Compiler::writeLhsRef( Production *production, ReduceTextItem *i )
{
	*outStream << "_lhs";
}

void Compiler::writeHostItemList( Production *production,
		const ReduceTextItemList &list )
{
	for ( ReduceTextItemList::Iter i = list; i.lte(); i++ ) {
		switch ( i->type ) {
			case ReduceTextItem::LhsRef:
				writeLhsRef( production, i );
				break;
			case ReduceTextItem::RhsRef:
				writeRhsRef( production, i );
				break;
			case ReduceTextItem::RhsLoc:
				writeRhsLoc( production, i );
				break;
			case ReduceTextItem::Txt:
				*outStream << i->txt;
				break;
		}
	}
}

/* For sorting according to prod name id, then by prod num. */
struct CmpReduceAction
{
	static int compare( const ReduceAction *ra1 , const ReduceAction *ra2 )
	{
		if ( ra1->production->prodName->id < ra2->production->prodName->id )
			return -1;
		else if ( ra1->production->prodName->id > ra2->production->prodName->id )
			return 1;
		else {
			if ( ra1->production->prodNum < ra2->production->prodNum )
				return -1;
			else if ( ra1->production->prodNum < ra2->production->prodNum )
				return 1;
		}
		return 0;
	}
};

void Compiler::initReductionNeeds( Reduction *reduction )
{
	reduction->needData = new bool[nextLelId];
	reduction->needLoc = new bool[nextLelId];
	memset( reduction->needData, 0, sizeof(bool)*nextLelId );
	memset( reduction->needLoc, 0, sizeof(bool)*nextLelId );
}

void Compiler::writeNeeds()
{

	*outStream <<
		"struct reduction_info\n"
		"{\n"
		"	unsigned char need_data[" << nextLelId << "];\n"
		"	unsigned char need_loc[" << nextLelId << "];\n"
		"};\n"
		"\n";

	*outStream <<
		"static struct reduction_info ri[" << rootNamespace->reductions.length() + 1 << "];\n"
		"\n";

	*outStream <<
		"extern \"C\" void " << objectName << "_init_need()\n"
		"{\n";
	
	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		Reduction *reduction = *r;
		*outStream <<
			"	memset( ri[" << reduction->id << "]"
					".need_data, 0, sizeof(unsigned char) * " << nextLelId << " );\n"
			"	memset( ri[" << reduction->id << "]"
					".need_loc, 0, sizeof(unsigned char) * " << nextLelId << " );\n";

		for ( int i = 0; i < nextLelId; i++ ) {
			if ( reduction->needData[i] ) {
				*outStream <<
					"	ri[" << reduction->id << "].need_data[" << i << "] = COLM_RN_DATA;\n";
			}

			if ( reduction->needLoc[i] ) {
				*outStream <<
					"	ri[" << reduction->id << "].need_loc[" << i << "] = COLM_RN_LOC;\n";
			}
		}
	}

	*outStream <<
		"}\n";

	*outStream <<
		"extern \"C\" int " << objectName << "_reducer_need_tok( program_t *prg, "
				"struct pda_run *pda_run, int id )\n"
		"{\n"
		"	if ( pda_run->reducer > 0 ) {\n"
		/* Note we are forcing the reducer need for data. Enabling requires finding
		 * a solution for backtracking push. */
		"		return COLM_RN_DATA | ri[pda_run->reducer].need_data[id] | \n"
		"			ri[pda_run->reducer].need_loc[id];\n"
		"	}\n"
		"	return COLM_RN_BOTH;\n"
		"}\n"
		"\n"
		"extern \"C\" int " << objectName << "_reducer_need_ign( program_t *prg, struct pda_run *pda_run )\n"
		"{\n"
		// Using this requires finding a solution for backtracking push back.
		//"	if ( pda_run->reducer > 0 )\n"
		//"		return COLM_RN_NEITHER;\n"
		"	return COLM_RN_BOTH;\n"
		"}\n";
}

void Compiler::writeReduceStructs()
{
	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		for ( ReduceNonTermList::Iter rdi = (*r)->reduceNonTerms; rdi.lte(); rdi++ ) {
			*outStream <<
				"struct lel_" << rdi->nonTerm->uniqueType->langEl->fullName << "\n"
				"{\n";

			*outStream <<
				"#line " << rdi->loc.line << "\"" << rdi->loc.fileName << "\"\n";

			writeHostItemList( 0, rdi->itemList );

			*outStream <<
				"};\n";
		}
	}

	*outStream << 
		"union commit_reduce_union\n"
		"{\n";

	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		for ( ReduceNonTermList::Iter rdi = (*r)->reduceNonTerms; rdi.lte(); rdi++ ) {
			LangEl *langEl = rdi->nonTerm->uniqueType->langEl;
			*outStream <<
				"	lel_" << langEl->fullName << " " << langEl->fullName << ";\n";
		}
	}

	*outStream <<
		"};\n"
		"\n";

	*outStream <<
		"extern \"C\" long " << objectName << "_commit_union_sz( int reducer )\n"
		"{\n"
		"	return sizeof( commit_reduce_union );\n"
		"}\n";

	*outStream <<
		"struct read_reduce_node\n"
		"{\n"
		"	std::string name;\n"
		"	int id;\n"
		"	int prod_num;\n"
		"	colm_location loc;\n"
		"	colm_data data;\n"
		"	commit_reduce_union u;\n"
		"	read_reduce_node *next;\n"
		"	read_reduce_node *child;\n"
		"};\n"
		"\n"
		"static void unescape( colm_data *tokdata )\n"
		"{\n"
		"	unsigned char *src = (unsigned char*)tokdata->data, *dest = (unsigned char*)tokdata->data;\n"
		"	while ( *src != 0 ) {\n"
		"		if ( *src == '\\\\' ) {\n"
		"			unsigned int i;\n"
		"			char buf[3];\n"
		"\n"
		"			src += 1;\n"
		"			buf[0] = *src++;\n"
		"			buf[1] = *src++;\n"
		"			buf[2] = 0;\n"
		"\n"
		"			sscanf( buf, \"%x\", &i );\n"
		"			*dest++ = (unsigned char)i;\n"
		"\n"
		"			tokdata->length -= 2;\n"
		"		}\n"
		"		else {\n"
		"			*dest++ = *src++;\n"
		"		}\n"
		"	}\n"
		"	*dest = 0;\n"
		"}\n"
		"\n";
}

void Compiler::writeReduceDispatchers()
{
	*outStream <<
		"\n"
		"extern \"C\" void " << objectName << "_commit_reduce_forward( program_t *prg, tree_t **root,\n"
		"		struct pda_run *pda_run, parse_tree_t *pt )\n"
		"{\n"
		"	switch ( pda_run->reducer ) {\n";

	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		Reduction *reduction = *r;
		if ( reduction->parserBased ) {
			*outStream <<
				"	case " << reduction->id << ":\n"
				"		((" << reduction->name << "*)prg->red_ctx)->commit_reduce_forward( "
							"prg, root, pda_run, pt );\n"
				"		break;\n";
		}
	}

	*outStream <<
		"	}\n"
		"}\n"
		"\n";

	*outStream <<
		"extern \"C\" void " << objectName << "_read_reduce( program_t *prg, int reducer, stream_t *stream )\n"
		"{\n"
		"	switch ( reducer ) {\n";

	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		Reduction *reduction = *r;
		if ( reduction->postfixBased ) {
			*outStream <<
				"	case " << reduction->id << ":\n"
				"		((" << reduction->name << "*)prg->red_ctx)->read_reduce_forward( prg, stream->impl->file );\n"
				"		break;\n";
		}
	}

	*outStream <<
		"	}\n"
		"}\n"
		"\n";
}

void Compiler::computeNeeded()
{
	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		Reduction *reduction = *r;
		initReductionNeeds( reduction );

		for ( ReduceActionList::Iter rdi = reduction->reduceActions; rdi.lte(); rdi++ )
			computeNeeded( reduction, rdi->production, rdi->itemList );
	}
}

void Compiler::writeParseReduce( Reduction *reduction )
{
	*outStream <<
		"void " << reduction->name << "::commit_reduce_forward( program_t *prg, \n"
		"		tree_t **root, struct pda_run *pda_run, parse_tree_t *pt )\n"
		"{\n"
		"	tree_t **sp = root;\n"
		"\n"
		"	parse_tree_t *lel = pt;\n"
		"	kid_t *kid = pt->shadow;\n"
		"\n"
		"recurse:\n"
		"\n"
		"	if ( lel->child != 0 ) {\n"
		"		/* There are children. Must process all children first. */\n"
		"		vm_push_ptree( lel );\n"
		"		vm_push_kid( kid );\n"
		"\n"
		"		lel = lel->child;\n"
		"		kid = tree_child( prg, kid->tree );\n"
		"		while ( lel != 0 ) {\n"
		"			goto recurse;\n"
		"			resume:\n"
		"			lel = lel->next;\n"
		"			kid = kid->next;\n"
		"		}\n"
		"\n"
		"		kid = vm_pop_kid();\n"
		"		lel = vm_pop_ptree();\n"
		"	}\n"
		"\n"
		"	if ( !( lel->flags & PF_COMMITTED ) ) {\n"
		"		/* Now can execute the reduction action. */\n"
		"		{\n";


	*outStream <<
		"		{ switch ( kid->tree->id ) {\n";

	/* Populate a vector with the reduce actions. */
	Vector<ReduceAction*> actions;
	actions.setAsNew( reduction->reduceActions.length() );
	long pos = 0;
	for ( ReduceActionList::Iter rdi = reduction->reduceActions; rdi.lte(); rdi++ )
		actions[pos++] = rdi;

	/* Sort it by lhs id, then prod num. */
	MergeSort<ReduceAction*, CmpReduceAction> sortActions;
	sortActions.sort( actions.data, actions.length() );

	ReduceAction *last = 0;

	for ( Vector<ReduceAction*>::Iter rdi = actions; rdi.lte(); rdi++ ) {
		ReduceAction *action = *rdi;
		int lelId = action->production->prodName->id;
		int prodNum = action->production->prodNum;

		/* Maybe close off the last prod. */
		if ( last != 0 && 
				last->production->prodName != action->production->prodName )
		{
			*outStream <<
			"			break;\n"
			"		}\n";
				
		}

		/* Maybe open a new prod. */
		if ( last == 0 || 
				last->production->prodName != action->production->prodName )
		{
			*outStream <<
				"		case " << lelId << ": {\n";
		}

		*outStream << 
			"			if ( kid->tree->prod_num == " << prodNum << " ) {\n";


		loadRefs( reduction, action->production, action->itemList, false );

		*outStream <<
			"#line " << action->loc.line << " \"" << action->loc.fileName << "\"\n";

		writeHostItemList( action->production, action->itemList );

		*outStream << 
			"			}\n";

		last = action;
	}

	if ( last != 0 ) {
		*outStream <<
			"			break;\n"
			"		}\n";
	}

	*outStream <<
		"		} }\n"
		"		}\n"
		"	}\n"
		"\n"
		"	commit_clear_parse_tree( prg, sp, pda_run, lel->child );\n"
		"	commit_clear_kid_list( prg, sp, kid->tree->child );\n"
		"	kid->tree->child = 0;\n"
		"	kid->tree->flags &= ~( AF_LEFT_IGNORE | AF_RIGHT_IGNORE );\n"
		"	lel->child = 0;\n"
		"\n"
		"	if ( sp != root )\n"
		"		goto resume;\n"
		"	pt->flags |= PF_COMMITTED;\n"
		"}\n"
		"\n";

}

void Compiler::writeParseReduce()
{
	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		Reduction *reduction = *r;
		if ( reduction->parserBased )
			writeParseReduce( reduction );
	}
}

void Compiler::writePostfixReduce( Reduction *reduction )
{
	*outStream <<
		"void " << reduction->name << "::read_reduce_forward( program_t *prg, FILE *file )\n"
		"{\n"
		"	__gnu_cxx::stdio_filebuf<char> fbuf( file, std::ios::in|std::ios::out|std::ios::app );\n"
		"	std::iostream in( &fbuf );\n"
		"	std::string type, tok, text;\n"
		"	long _id, line, column, byte, prod_num, children;\n"
		"	read_reduce_node sentinal;\n"
		"	sentinal.next = 0;\n"
		"	read_reduce_node *stack = &sentinal, *last = 0;\n"
		"	while ( in >> type ) {\n"
		"		/* read. */\n"
		"		if ( type == \"t\" ) {\n"
		"			in >> tok >> _id >> line >> column >> byte >> text;\n"
		"			read_reduce_node *node = new read_reduce_node;\n"
		"			node->name = tok;\n"
		"			node->id = _id;\n"
		"			node->loc.name = \"<>\";\n"
		"			node->loc.line = line;\n"
		"			node->loc.column = column;\n"
		"			node->loc.byte = byte;\n"
		"			node->data.data = strdup( text.c_str() );\n"
		"			node->data.length = text.size();\n"
		"			unescape( &node->data );\n"
		"\n"
		"			node->next = stack;\n"
		"			node->child = 0;\n"
		"			stack = node;\n"
		"		}\n"
		"		else if ( type == \"r\" ) {\n"
		"			in >> tok >> _id >> prod_num >> children;\n"
		"			read_reduce_node *node = new read_reduce_node;\n"
		"			memset( &node->loc, 0, sizeof(colm_location) );\n"
		"			memset( &node->data, 0, sizeof(colm_data) );\n"
		"			node->name = tok;\n"
		"			node->id = _id;\n"
		"			node->prod_num = prod_num;\n"
		"			node->child = 0;\n"
		"			while ( children-- > 0 ) {\n"
		"				last = stack;\n"
		"				stack = stack->next;\n"
		"				last->next = node->child;\n"
		"				node->child = last;\n"
		"			}\n"
		"\n"
		"			node->next = stack;\n"
		"			stack = node;\n"
		"\n"
		"			{ switch ( node->id ) {\n";

	/* Populate a vector with the reduce actions. */
	Vector<ReduceAction*> actions;
	actions.setAsNew( reduction->reduceActions.length() );
	long pos = 0;
	for ( ReduceActionList::Iter rdi = reduction->reduceActions; rdi.lte(); rdi++ )
		actions[pos++] = rdi;

	/* Sort it by lhs id, then prod num. */
	MergeSort<ReduceAction*, CmpReduceAction> sortActions;
	sortActions.sort( actions.data, actions.length() );

	ReduceAction *last = 0;

	for ( Vector<ReduceAction*>::Iter rdi = actions; rdi.lte(); rdi++ ) {
		ReduceAction *action = *rdi;
		int lelId = action->production->prodName->id;
		int prodNum = action->production->prodNum;

		/* Maybe close off the last prod. */
		if ( last != 0 && 
				last->production->prodName != action->production->prodName )
		{
			*outStream <<
			"			break;\n"
			"		}\n";
		}

		/* Maybe open a new prod. */
		if ( last == 0 || 
				last->production->prodName != action->production->prodName )
		{
			*outStream <<
				"		case " << lelId << ": {\n";
		}

		*outStream << 
			"			if ( node->prod_num == " << prodNum << " ) {\n";

		loadRefs( reduction, action->production, action->itemList, true );

		*outStream <<
			"#line " << action->loc.line << "\"" << action->loc.fileName << "\"\n";

		writeHostItemList( action->production, action->itemList );

		*outStream << 
			"			}\n";

		last = action;
	}

	if ( last != 0 ) {
		*outStream <<
			"			break;\n"
			"		}\n";
	}

	*outStream <<
		"			} }\n"
		"			/* delete the children */\n"
		"			last = node->child;\n"
		"			while ( last != 0 ) {\n"
		"				read_reduce_node *next = last->next;\n"
		"				delete last;\n"
		"				last = next;\n"
		"			}\n"
		"		}\n"
		"	}\n"
		"}\n"
		"\n";
}

void Compiler::writePostfixReduce()
{
	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		Reduction *reduction = *r;
		if ( reduction->postfixBased )
			writePostfixReduce( reduction );
	}
}

void Compiler::writeCommit()
{
	*outStream <<
		"#include <colm/pdarun.h>\n"
		"#include <colm/bytecode.h>\n"
		"#include <colm/defs.h>\n"
		"#include <colm/input.h>\n"
		"#include <colm/tree.h>\n"
		"#include <colm/program.h>\n"
		"#include <colm/colm.h>\n"
		"\n"
		"#include <stdio.h>\n"
		"#include <stdlib.h>\n"
		"#include <string.h>\n"
		"#include <assert.h>\n"
		"#include <errno.h>\n"
		"\n"
		"#include <iostream>\n"
		"#include <ext/stdio_filebuf.h>\n"
		"#include <fstream>\n"
		"\n"
		"using std::endl;\n"
		"\n"
		"#include \"reducer.h\"\n"
		"\n";

	computeNeeded();

	writeReduceStructs();
	
	writeReduceDispatchers();

	writePostfixReduce();

	writeParseReduce();

	writeNeeds();
}
