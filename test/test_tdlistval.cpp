/*
 *  Copyright 2003 Adrian Thurston <thurston@cs.queensu.ca>
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
#include "tdlist.h"
#include "tdlistmel.h"
#include "tdlistval.h"

using namespace std;

#define TD

#ifdef TD
typedef TDListVal<char> List1;
typedef TDListVal<unsigned char> List2;
typedef TDListVal<short> List3;
typedef TDListVal<unsigned short> List4;
typedef TDListVal<int> List5;
typedef TDListVal<unsigned int> List6;
#else
typedef DListVal<char> List1;
typedef DListVal<unsigned char> List2;
typedef DListVal<short> List3;
typedef DListVal<unsigned short> List4;
typedef DListVal<int> List5;
typedef DListVal<unsigned int> List6;
#endif

void testTDList1()
{
	List1 list;
	list.append( 1 );
	List1::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << *it << endl;
	List1 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}

void testTDList2()
{
	List2 list;
	list.append( 1 );
	List2::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << *it << endl;
	List2 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}


void testTDList3()
{
	List3 list;
	list.append( 1 );
	List3::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << *it << endl;
	List3 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}


void testTDList4()
{
	List4 list;
	list.append( 1 );
	List4::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << *it << endl;
	List4 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}


void testTDList5()
{
	List5 list;
	list.append( 1 );
	List5::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << *it << endl;
	List5 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}

void testTDList6()
{
	List6 list;
	list.append( 1 );
	List6::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << *it << endl;
	List6 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}

int main()
{
	testTDList1();
	testTDList2();
	testTDList3();
	testTDList4();
	testTDList5();
	testTDList6();
	return 0;
}
