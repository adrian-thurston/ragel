/*
 * Copyright 2003 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
