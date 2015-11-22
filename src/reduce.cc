/*
 *  Copyright 2015 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <iostream>
#include <iomanip>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <sstream>

#include "global.h"
#include "avltree.h"
#include "compiler.h"
#include "parser.h"
#include "parsetree.h"
#include "mergesort.h"
#include "redbuild.h"
#include "pdacodegen.h"
#include "fsmcodegen.h"
#include "pdarun.h"
#include "colm.h"
#include "pool.h"
#include "struct.h"

void Compiler::writeCommitStub()
{
	*outStream <<
		"void commit_reduce_forward( program_t *prg, tree_t **root,\n"
		"		struct pda_run *pda_run, parse_tree_t *pt )\n"
		"{\n"
		"	commit_clear_parse_tree( prg, root, pda_run, pt->child );\n"
		"}\n"
		"\n"
		"long commit_union_sz( int reducer ) { return 0; }\n"
		"\n";
	;
}

void Compiler::loadRefs( Production *production, const ReduceTextItemList &list )
{
	ObjectDef *objectDef = production->prodName->objectDef;
	Vector<ProdEl*> rhsUsed;
	Vector<ProdEl*> locUsed;
	rhsUsed.setAsNew( production->prodElList->length() );
	locUsed.setAsNew( production->prodElList->length() );

	bool lhsUsed = false;
	for ( ReduceTextItemList::Iter i = list; i.lte(); i++ ) {
		if ( i->type == ReduceTextItem::LhsRef ) {
			lhsUsed = true;
		}

		if ( i->type == ReduceTextItem::RhsRef || i->type == ReduceTextItem::RhsLoc ) {
			if ( i->n > 0 ) {
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

	if ( lhsUsed ) {
		*outStream <<
			"	lel_" << production->prodName->fullName << " *_lhs = "
				"&((commit_reduce_union*)(lel+1))->" <<
				production->prodName->fullName << ";\n";
	}

	for ( Vector<ProdEl*>::Iter rhs = rhsUsed; rhs.lte(); rhs++ ) {
		if ( *rhs != 0 ) {
			ProdEl *prodEl = *rhs;
			if ( prodEl->production == production ) {
				if ( prodEl->langEl->type == LangEl::Term ) {
					*outStream <<
						"	colm_data *_rhs" << rhs.pos() << " = "
							"get_rhs_el_kid( prg, kid->tree, "
							<< prodEl->pos << " )->tree->tokdata;\n";
				}
				else {
					*outStream <<
							"lel_" << prodEl->langEl->fullName << " *"
							"_rhs" << rhs.pos() << " = &((commit_reduce_union*)"
							"(get_rhs_parse_tree( prg, lel, " <<
							prodEl->pos << " )+1))->" <<
							prodEl->langEl->fullName << ";\n";
				}
			}

		}
	}
	for ( Vector<ProdEl*>::Iter loc = locUsed; loc.lte(); loc++ ) {
		ProdEl *prodEl = *loc;
		if ( prodEl != 0 ) {
			if ( prodEl->production == production ) {
				*outStream <<
					"	colm_location *_loc" << loc.pos() << " = "
						"colm_find_location( prg, get_rhs_el_kid( prg, kid->tree, " <<
						prodEl->pos << " )->tree );\n";
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
		"using std::endl;\n"
		"\n"
		"#include \"reducer.h\"\n"
		"\n";

	*outStream << "\n";

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
		"long commit_union_sz( int reducer )\n"
		"{\n"
		"	return sizeof( commit_reduce_union );\n"
		"}\n";

	*outStream <<
		"\n"
		"void commit_reduce_forward( program_t *prg, tree_t **root,\n"
		"		struct pda_run *pda_run, parse_tree_t *pt )\n"
		"{\n"
		"	switch ( pda_run->reducer ) {\n";

	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		Reduction *reduction = *r;
		*outStream <<
			"	case " << reduction->id << ":\n"
			"		((" << reduction->name << "*)prg->red_ctx)->commit_reduce_forward( "
						"prg, root, pda_run, pt );\n"
			"		break;\n";
	}

	*outStream <<
		"	}\n"
		"}\n"
		"\n";

	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		Reduction *reduction = *r;

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
			"		kid = kid->tree->child;\n"
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

			loadRefs( action->production, action->itemList );

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
			"		} }\n"
			"		}\n"
			"	}\n"
			"\n"
			"	commit_clear_parse_tree( prg, sp, pda_run, lel->child );\n"
			"	lel->child = 0;\n"
			"\n"
			"	if ( sp != root )\n"
			"		goto resume;\n"
			"	pt->flags |= PF_COMMITTED;\n"
			"}\n"
			"\n";
	}
}
