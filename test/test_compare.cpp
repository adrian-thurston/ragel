/*
 *  Copyright 2005 Adrian Thurston <thurston@cs.queensu.ca>
 */

/*  This file is part of Aapl.
 *
 *  Aapl is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU Lesser General Public License as published by the Free
 *  Software Foundation; either version 2.1 of the License, or (at your option)
 *  any later version.
 *
 *  Aapl is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Aapl; if not, write to the Free Software Foundation, Inc., 59
 *  Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <iostream>
#include <assert.h>

#include "vector.h"
#include "compare.h"
#include "bstset.h"

using namespace std;

struct Item
{
	int i;
};

struct CmpItem
{
	int compareData;
	int compare( const Item &k1, const Item &k2 )
	{
		if ( k1.i < k2.i )
			return -1;
		else if ( k1.i > k2.i )
			return 1;
		else
			return 0;
	}
};

int main()
{
	Vector<Item> v;
	BstSet< Vector<Item>, CmpTableNs<Item, CmpItem> > bst;
	bst.insert(Vector<Item>());
}
