/*
 *  Copyright 2001 Adrian Thurston <thurston@cs.queensu.ca>
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
#include "dlist.h"
#include "dlistmel.h"
#include "dlistval.h"

using std::cout;
using std::endl;

/*
 * DListMel
 */
struct obj;
class MyListEl1 : public DListEl<obj>  { };
class MyListEl2 : public DListEl<obj>  { };
struct obj : public MyListEl1, public MyListEl2
{
	int data;
};
template class DListMel< obj, MyListEl2 >;

/*
 * DList
 */
struct SingleObj : public DListEl<SingleObj> { };
template class DList< SingleObj >;

/*
 * DListVal
 */
template class DListVal< int >;

struct Integer
{
	Integer(const int &i) : i(i) { }
	Integer() : i(0) { }

	int i;
};

void testDoubleList()
{
	DListVal<Integer> dl;
	Integer i = 0;
	dl.append(Integer(1));
	dl.prepend( i );
	dl.addAfter( dl.head, Integer(22) );

	DListVal<Integer>::Iter it = dl;
	for ( it = dl; ! it.end(); it++ )
		printf("%i\n", it->value.i);
}

void testConstructors1()
{
	DListVal<int> dl;
	dl.append( 1 );
	dl.append( 2 );
	DListVal<int> copy(dl);
	dl.empty();

	for ( DListVal<int>::Iter i = copy; i.lte(); i++ )
		cout << i->value << endl;
	dl.empty();
}

void testConstructors2()
{
	DListVal<int> dl;
	dl.append( 1 );
	dl.append( 2 );
	DListVal<int> copy(dl);
	dl.empty();

	for ( DListVal<int>::Iter i = copy; i.lte(); i++ )
		cout << i->value << endl;

	dl.empty();
}

int main()
{
	testDoubleList();
	testConstructors1();
	testConstructors2();
	return 0;
}
