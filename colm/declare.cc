/*
 *  Copyright 2011 Adrian Thurston <thurston@complang.org>
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

#include "bytecode.h"
#include "parsedata.h"
#include "fsmrun.h"
#include <iostream>
#include <assert.h>

void Namespace::declare( ParseData *pd )
{
	for ( GenericList::Iter g = genericList; g.lte(); g++ ) {
		//std::cout << "generic " << g->name << std::endl;

		KlangEl *langEl = getKlangEl( pd, this, g->name );

		/* Check that the element wasn't previously defined as something else. */
		if ( langEl->type != KlangEl::Unknown ) {
			error() << "'" << g->name << 
				"' already defined as something else" << endp;
		}
		langEl->type = KlangEl::NonTerm;

		/* Add one empty production. */
		ProdElList *emptyList = new ProdElList;
		//addProduction( g->loc, langEl, emptyList, false, 0, 0 );

		{
			KlangEl *prodName = langEl;
			assert( prodName->type == KlangEl::NonTerm );

			Definition *newDef = new Definition( loc, prodName, 
				emptyList, false, 0,
				pd->prodList.length(), Definition::Production );
			
			prodName->defList.append( newDef );
			pd->prodList.append( newDef );
			newDef->predOf = 0;
		}

		langEl->generic = g;
		g->langEl = langEl;
	}

	for ( LiteralDict::Iter l = literalDict; l.lte(); l++  ) {
		/* Create a token for the literal. */
		KlangEl *newLangEl = getKlangEl( pd, this, l->value->name );
		assert( newLangEl->type == KlangEl::Unknown );
		newLangEl->type = KlangEl::Term;
		newLangEl->lit = l->value->literal;
		newLangEl->isLiteral = true;
		newLangEl->tokenDef = l->value;

		l->value->token = newLangEl;
	}

	for ( ContextDefList::Iter c = contextDefList; c.lte(); c++ ) {
		KlangEl *lel = getKlangEl( pd, this, c->name );

		/* Check that the element wasn't previously defined as something else. */
		if ( lel->type != KlangEl::Unknown ) {
			error(c->context->loc) << "'" << c->name << 
				"' has already been defined, maybe you want to use redef?" << endp;
		}
		lel->type = KlangEl::NonTerm;
		ProdElList *emptyList = new ProdElList;
		//addProduction( c->context->loc, c->name, emptyList, false, 0, 0 );

		{
			KlangEl *prodName = lel;
			assert( prodName->type == KlangEl::NonTerm );

			Definition *newDef = new Definition( loc, prodName, 
				emptyList, false, 0,
				pd->prodList.length(), Definition::Production );
			
			prodName->defList.append( newDef );
			pd->prodList.append( newDef );
			newDef->predOf = 0;

			/* If the token has the same name as the region it is in, then also
			 * insert it into the symbol map for the parent region. */
			if ( strcmp( c->name, this->name ) == 0 ) {
				/* Insert the name into the top of the region stack after popping the
				 * region just created. We need it in the parent. */
				this->parentNamespace->symbolMap.insert( c->name, prodName );
			}
		}

		c->context->lel = lel;
		lel->contextDef = c->context;
		lel->objectDef = c->context->contextObjDef;
	}

	for ( TokenDefListNs::Iter t = tokenDefList; t.lte(); t++ ) {
		/* Literals already taken care of. */
		if ( ! t->isLiteral ) {
			/* Create the token. */
			KlangEl *tokEl = getKlangEl( pd, this, t->name );
			if ( tokEl->type != KlangEl::Unknown )
				error(InputLoc()) << "'" << t->name << "' already defined" << endp;

			tokEl->type = KlangEl::Term;
			tokEl->ignore = t->ignore;
			tokEl->transBlock = t->codeBlock;
			tokEl->objectDef = t->objectDef;
			tokEl->contextIn = t->contextIn;
			tokEl->tokenDef = t;

			t->token = tokEl;
		}
	}

	for ( NtDefList::Iter n = ntDefList; n.lte(); n++ ) {
		/* Get the language element. */
		KlangEl *langEl = getKlangEl( pd, this, n->name );

		/* Check that the element wasn't previously defined as something else. */
		if ( langEl->type != KlangEl::Unknown ) {
			error(InputLoc()) << "'" << n->name << 
				"' has already been defined, maybe you want to use redef?" << endp;
		}

		langEl->type = KlangEl::NonTerm;
		//$$->langEl = langEl;

		/* Get the language element. */
		langEl->objectDef = n->objectDef;
		langEl->reduceFirst = n->reduceFirst;
		langEl->contextIn = n->contextIn;
		langEl->defList.transfer( *n->defList );

		for ( LelDefList::Iter d = langEl->defList; d.lte(); d++ ) {
			d->prodName = langEl;

			if ( d->redBlock != 0 ) {
				pd->addProdRedObjectVar( d->redBlock->localFrame, langEl );
				pd->addProdRHSVars( d->redBlock->localFrame, d->prodElList );
			}

		/* References to the reduce item. */
		}
	}

	for ( NamespaceVect::Iter c = childNamespaces; c.lte(); c++ ) {
		//std::cout << "namespace " << (*c)->name << std::endl;
		(*c)->declare( pd );
	}
}

