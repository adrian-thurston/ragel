/*
 *  Copyright 2003 Adrian Thurston <adriant@ragel.ca>
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
#include "dlist.h"

using namespace std;

#define TD

#ifdef TD
struct ListEl1 : public TDListEl { int i; };
struct ListEl2 : public TDListEl { int i; };
struct ListEl3 : public TDListEl { int i; };
struct ListEl4 : public TDListEl { int i; };
struct ListEl5 : public TDListEl { int i; };
struct ListEl6 : public TDListEl { int i; };
typedef TDList<ListEl1> List1;
typedef TDList<ListEl2> List2;
typedef TDList<ListEl3> List3;
typedef TDList<ListEl4> List4;
typedef TDList<ListEl5> List5;
typedef TDList<ListEl6> List6;
#else
struct ListEl1 : public DListEl<ListEl1> { int i; };
struct ListEl2 : public DListEl<ListEl2> { int i; };
struct ListEl3 : public DListEl<ListEl3> { int i; };
struct ListEl4 : public DListEl<ListEl4> { int i; };
struct ListEl5 : public DListEl<ListEl5> { int i; };
struct ListEl6 : public DListEl<ListEl6> { int i; };
typedef DList<ListEl1> List1;
typedef DList<ListEl2> List2;
typedef DList<ListEl3> List3;
typedef DList<ListEl4> List4;
typedef DList<ListEl5> List5;
typedef DList<ListEl6> List6;
#endif


void testTDList1()
{
	List1 list;
	ListEl1 lel;
	list.append( &lel );
	List1::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << it->i << endl;
	List1 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}

void testTDList2()
{
	List2 list;
	ListEl2 lel;
	list.append( &lel );
	List2::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << it->i << endl;
	List2 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}


void testTDList3()
{
	List3 list;
	ListEl3 lel;
	list.append( &lel );
	List3::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << it->i << endl;
	List3 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}


void testTDList4()
{
	List4 list;
	ListEl4 lel;
	list.append( &lel );
	List4::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << it->i << endl;
	List4 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}


void testTDList5()
{
	List5 list;
	ListEl5 lel;
	list.append( &lel );
	List5::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << it->i << endl;
	List5 copy( list );
	copy.empty();
	cout << list.head << endl;
	cout << copy.head << endl;
}

void testTDList6()
{
	List6 list;
	ListEl6 lel;
	list.append( &lel );
	List6::Iter it = list.first();
	for ( ; it.lte(); it++ )
		cout << it->i << endl;
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
