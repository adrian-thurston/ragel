/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
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
#include <string>
#include <errno.h>

#include "parser.h"
#include "config.h"
#include "lmparse.h"
#include "global.h"
#include "input.h"
#include "bootstrap1.h"
#include "exports1.h"
#include "colm/colm.h"

using std::string;

extern RuntimeData main_runtimeData;

void Bootstrap1::prodElList( ProdElList *list, prod_el_list &ProdElList )
{
	if ( ProdElList.ProdElList() != 0 ) {
		prod_el_list RightProdElList = ProdElList.ProdElList();
		prodElList( list, RightProdElList );
	}
	
	if ( ProdElList.ProdEl() != 0 ) {
		std::cout << "prod el: " << ProdElList.ProdEl().text() << std::endl;
		String name = ProdElList.ProdEl().text().c_str();

		ProdEl *prodEl = prodElName( internal, name,
				NamespaceQual::cons(namespaceStack.top()), 0,
				RepeatNone, false );

		appendProdEl( list, prodEl );
	}
}

void Bootstrap1::prodList( LelDefList *lelDefList, prod_list &ProdList )
{
	if ( ProdList.ProdList() != 0 ) {
		prod_list RightProdList = ProdList.ProdList();
		prodList( lelDefList, RightProdList );
	}

	ProdElList *list = new ProdElList;
	
	std::cout << "prod: " << ProdList.Prod().text() << std::endl;
	prod_el_list ProdElList = ProdList.Prod().ProdElList();
	prodElList( list, ProdElList );

	Production *prod = BaseParser::production( internal, list, false, 0, 0 );
	prodAppend( lelDefList, prod );
}

void Bootstrap1::defineProd( item &Define )
{
	prod_list ProdList = Define.ProdList();

	LelDefList *defList = new LelDefList;
	prodList( defList, ProdList );

	String name = Define.DefId().text().c_str();
	NtDef *ntDef = NtDef::cons( name, namespaceStack.top(), contextStack.top(), false );
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 
	cflDef( ntDef, objectDef, defList );
}

void Bootstrap1::go()
{
	StmtList *stmtList = new StmtList;

	colmInit( 0 );
	ColmProgram *program = colmNewProgram( &main_runtimeData );
	colmRunProgram( program, 0, 0 );

	/* Extract the parse tree. */
	start Start = Colm0Tree( program );

	if ( Start == 0 ) {
		std::cerr << "error parsing input" << std::endl;
		return;
	}

	/* Walk the list of items. */
	_repeat_item ItemList = Start.ItemList();
	while ( !ItemList.end() ) {

		item Item = ItemList.value();
		if ( Item.DefId() != 0 ) {
			// std::cout << "define: " << Item.text() << std::endl;
			std::cout << "define-id: " << Item.DefId().text() << std::endl;
			defineProd( Item );
		}
		else {
			//std::cout << "other:  " << Item.text() << std::endl;
		}
		ItemList = ItemList.next();
	}

	colmDeleteProgram( program );

	//parseInput( stmtList );
	//exportTree( stmtList );

	pd->rootCodeBlock = CodeBlock::cons( stmtList, 0 );
}

