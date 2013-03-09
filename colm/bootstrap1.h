/*
 *  Copyright 2013 Adrian Thurston <thurston@complang.org>
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
#include "avltree.h"
#include "parsedata.h"
#include "parser.h"
#include "bootstrap.h"

struct token_list;
struct prod_el_list;
struct prod_list;
struct item;

struct Bootstrap1
:
	public BootstrapBase
{
	Bootstrap1( Compiler *pd )
	:
		BootstrapBase(pd)
	{
	}

	void tokenList( token_list &TokenList );
	void lexRegion( item &LexRegion );
	void prodElList( ProdElList *list, prod_el_list &ProdElList );
	void prodList( LelDefList *lelDefList, prod_list &ProdList );
	void defineProd( item &Define );
	void go();
};
