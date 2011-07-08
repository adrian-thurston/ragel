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

void ParseData::declareBaseKlangEls()
{
	/* Make the "stream" language element */
	streamKlangEl = new LangEl( rootNamespace, strdup("stream"), LangEl::Term );
	langEls.prepend( streamKlangEl );
	SymbolMapEl *streamMapEl = rootNamespace->symbolMap.insert( 
			streamKlangEl->name, streamKlangEl );
	assert( streamMapEl != 0 );

	/* Make the "str" language element */
	strKlangEl = new LangEl( rootNamespace, strdup("str"), LangEl::Term );
	langEls.prepend( strKlangEl );
	SymbolMapEl *stringMapEl = rootNamespace->symbolMap.insert( 
			strKlangEl->name, strKlangEl );
	assert( stringMapEl != 0 );

	/* Make the "int" language element */
	intKlangEl = new LangEl( rootNamespace, strdup("int"), LangEl::Term );
	langEls.prepend( intKlangEl );
	SymbolMapEl *integerMapEl = rootNamespace->symbolMap.insert( 
			intKlangEl->name, intKlangEl );
	assert( integerMapEl != 0 );

	/* Make the "bool" language element */
	boolKlangEl = new LangEl( rootNamespace, strdup("bool"), LangEl::Term );
	langEls.prepend( boolKlangEl );
	SymbolMapEl *boolMapEl = rootNamespace->symbolMap.insert( 
			boolKlangEl->name, boolKlangEl );
	assert( boolMapEl != 0 );

	/* Make the "ptr" language element */
	ptrKlangEl = new LangEl( rootNamespace, strdup("ptr"), LangEl::Term );
	langEls.prepend( ptrKlangEl );
	SymbolMapEl *ptrMapEl = rootNamespace->symbolMap.insert( 
			ptrKlangEl->name, ptrKlangEl );
	assert( ptrMapEl != 0 );

	/* Make a "_notoken" language element. This element is used when a
	 * generation action fails to generate anything, but there is reverse code
	 * that needs to be associated with a language element. This allows us to
	 * always associate reverse code with the first language element produced
	 * after a generation action. */
	noTokenKlangEl = new LangEl( rootNamespace, strdup("_notoken"), LangEl::Term );
	noTokenKlangEl->ignore = true;
	langEls.prepend( noTokenKlangEl );
	SymbolMapEl *noTokenMapEl = rootNamespace->symbolMap.insert( 
			noTokenKlangEl->name, noTokenKlangEl );
	assert( noTokenMapEl != 0 );

	/* Make the EOF language element. */
	eofKlangEl = 0;
//	eofKlangEl = new LangEl( rootNamespace, strdup("_eof"), LangEl::Term );
//	langEls.prepend( eofKlangEl );
//	SymbolMapEl *eofMapEl = rootNamespace->symbolMap.insert( eofKlangEl->name, eofKlangEl );
//	assert( eofMapEl != 0 );

	/* Make the "any" language element */
	anyKlangEl = new LangEl( rootNamespace, strdup("any"), LangEl::NonTerm );
	langEls.prepend( anyKlangEl );
	SymbolMapEl *anyMapEl = rootNamespace->symbolMap.insert( anyKlangEl->name, anyKlangEl );
	assert( anyMapEl != 0 );
}

void ParseData::makeTerminalWrappers()
{
	/* Make terminal language elements corresponding to each nonterminal in
	 * the grammar. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->type == LangEl::NonTerm ) {
			String name( lel->name.length() + 5, "_T_%s", lel->name.data );
			LangEl *termDup = new LangEl( lel->nspace, name, LangEl::Term );

			/* Give the dup the attributes of the nonterminal. This ensures
			 * that the attributes are allocated when patterns and
			 * constructors are parsed. */
			termDup->objectDef = lel->objectDef;

			langEls.append( termDup );
			lel->termDup = termDup;
			termDup->termDup = lel;
		}
	}
}

void ParseData::makeEofElements()
{
	/* Make eof language elements for each user terminal. This is a bit excessive and
	 * need to be reduced to the ones that we need parsers for, but we don't know that yet.
	 * Another pass before this one is needed. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->eofLel == 0 &&
				lel != eofKlangEl &&
				lel != errorKlangEl &&
				lel != noTokenKlangEl )
		{
			String name( lel->name.length() + 5, "_eof_%s", lel->name.data );
			LangEl *eofLel = new LangEl( lel->nspace, name, LangEl::Term );

			langEls.append( eofLel );
			lel->eofLel = eofLel;
			eofLel->eofLel = lel;
			eofLel->isEOF = true;
		}
	}
}

void ParseData::addProdRedObjectVar( ObjectDef *localFrame, LangEl *nonTerm )
{
	UniqueType *prodNameUT = findUniqueType( TYPE_TREE, nonTerm );
	TypeRef *typeRef = new TypeRef( InputLoc(), prodNameUT );
	ObjField *el = new ObjField( InputLoc(), typeRef, "lhs" );

	/* Is the only item pushed to the stack just before a reduction action is
	 * executed. We rely on a zero offset. */
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isLhsEl = true;
	el->offset = 0;

	initLocalInstructions( el );

	localFrame->insertField( el->name, el );
}

void ParseData::addProdRHSVars( ObjectDef *localFrame, ProdElList *prodElList )
{
	long position = 1;
	for ( ProdElList::Iter rhsEl = *prodElList; rhsEl.lte(); rhsEl++, position++ ) {
		if ( rhsEl->type == ProdEl::ReferenceType ) {
			/* Use an offset of zero. For frame objects we compute the offset on
			 * demand. */
			String name( 8, "r%d", position );
			ObjField *el = new ObjField( InputLoc(), rhsEl->typeRef, name );
			rhsEl->objField = el;

			/* Right hand side elements are constant. */
			el->isConst = true;
			el->isRhsEl = true;

			/* Only ever fetch for reading since they are constant. */
			el->inGetR = IN_GET_LOCAL_R;

			localFrame->insertField( el->name, el );
		}
	}
}

void ParseData::addProdRHSLoads( Definition *prod, CodeVect &code, long &insertPos )
{
	CodeVect loads;
	long elPos = 0;
	for ( ProdElList::Iter rhsEl = *prod->prodElList; rhsEl.lte(); rhsEl++, elPos++ ) {
		if ( rhsEl->type == ProdEl::ReferenceType ) {
			if ( rhsEl->objField->beenReferenced ) {
				loads.append ( IN_INIT_RHS_EL );
				loads.appendHalf( elPos );
				loads.appendHalf( rhsEl->objField->offset );
			}
		}
	}

	/* Insert and update the insert position. */
	code.insert( insertPos, loads );
	insertPos += loads.length();
}

void Namespace::declare( ParseData *pd )
{
	for ( GenericList::Iter g = genericList; g.lte(); g++ ) {
		//std::cout << "generic " << g->name << std::endl;

		LangEl *langEl = declareLangEl( pd, this, g->name );
		langEl->type = LangEl::NonTerm;

		/* Add one empty production. */
		ProdElList *emptyList = new ProdElList;
		//addProduction( g->loc, langEl, emptyList, false, 0, 0 );

		{
			LangEl *prodName = langEl;
			assert( prodName->type == LangEl::NonTerm );

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
		LangEl *newLangEl = declareLangEl( pd, this, l->value->name );
		newLangEl->type = LangEl::Term;
		newLangEl->lit = l->value->literal;
		newLangEl->isLiteral = true;
		newLangEl->tokenDef = l->value;

		l->value->token = newLangEl;
	}

	for ( ContextDefList::Iter c = contextDefList; c.lte(); c++ ) {
		LangEl *lel = declareLangEl( pd, this, c->name );
		lel->type = LangEl::NonTerm;
		ProdElList *emptyList = new ProdElList;
		//addProduction( c->context->loc, c->name, emptyList, false, 0, 0 );

		{
			LangEl *prodName = lel;
			assert( prodName->type == LangEl::NonTerm );

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
			LangEl *tokEl = declareLangEl( pd, this, t->name );
			tokEl->type = LangEl::Term;
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
		LangEl *langEl = declareLangEl( pd, this, n->name );
		langEl->type = LangEl::NonTerm;
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

