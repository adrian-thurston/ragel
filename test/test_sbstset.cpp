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

#include "sbstset.h"

using namespace std;

template class SBstSet< int, CmpOrd<int> >;

int main()
{
	SBstSet< int, CmpOrd<int> > set;
	set.insert(0);
	set.insert(0);
	set.insert(2);

	SBstSet< int, CmpOrd<int> >::Iter it;
	for ( it = set; !it.end(); it++ )
		cout << *it << endl;

	return 0;
}
