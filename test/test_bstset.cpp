/*
 *  Copyright 2001, 2002 Adrian Thurston <thurston@cs.queensu.ca>
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

#include <iostream>
#include <stdio.h>

#include "bstset.h"
#include "avlset.h"
using namespace std;

template class BstSet< int, CmpOrd<int> >;
template class AvlSet< int, CmpOrd<int> >;

int main()
{
	BstSet< int, CmpOrd<int> > set1;
	AvlSet< int, CmpOrd<int> > set2;

	set1.insert(0);
	set2.insert(0);

	set1.insert(0);
	set2.insert(0);

	set1.insert(2);
	set2.insert(2);

	BstSet< int, CmpOrd<int> >::Iter it1 = set1;
	AvlSet< int, CmpOrd<int> >::Iter it2 = set2;
	for ( ; it1.lte(); it1++, it2++ )
		cout << *it1 << " " << it2->key << endl;

	for ( int i = 0; i < 20000; i++ )
		set1.insert( i );

	int num = 0;
	for ( int i = 0; i < 20000; i++ ) {
		BstSet< int, CmpOrd<int> >::Iter iter = set1;
		for ( ; !iter.end(); iter++ )
			num += *iter;
	}
	cout << num << endl;

	return 0;
}
