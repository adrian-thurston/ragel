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
#include "tdlistmel.h"
#include "dlistmel.h"

using namespace std;

#define TD

#ifdef TD
struct ListEl1_A : public TDListEl { };
struct ListEl1_B : public TDListEl { };
struct ListEl2_A : public TDListEl { };
struct ListEl2_B : public TDListEl { };
struct ListEl3_A : public TDListEl { };
struct ListEl3_B : public TDListEl { };
struct ListEl4_A : public TDListEl { };
struct ListEl4_B : public TDListEl { };
struct ListEl5_A : public TDListEl { };
struct ListEl5_B : public TDListEl { };
struct ListEl6_A : public TDListEl { };
struct ListEl6_B : public TDListEl { };
struct ListEl1 : public ListEl1_A, public ListEl1_B { int i; };
struct ListEl2 : public ListEl2_A, public ListEl2_B { int i; };
struct ListEl3 : public ListEl3_A, public ListEl3_B { int i; };
struct ListEl4 : public ListEl4_A, public ListEl4_B { int i; };
struct ListEl5 : public ListEl5_A, public ListEl5_B { int i; };
struct ListEl6 : public ListEl6_A, public ListEl6_B { int i; };
typedef TDListMel<ListEl1, ListEl1_A> List1;
typedef TDListMel<ListEl2, ListEl2_A> List2;
typedef TDListMel<ListEl3, ListEl3_A> List3;
typedef TDListMel<ListEl4, ListEl4_A> List4;
typedef TDListMel<ListEl5, ListEl5_A> List5;
typedef TDListMel<ListEl6, ListEl6_A> List6;
#else
struct ListEl1;
struct ListEl2;
struct ListEl3;
struct ListEl4;
struct ListEl5;
struct ListEl1_A : public DListEl<ListEl1> { };
struct ListEl1_B : public DListEl<ListEl1> { };
struct ListEl2_A : public DListEl<ListEl2> { };
struct ListEl2_B : public DListEl<ListEl2> { };
struct ListEl3_A : public DListEl<ListEl3> { };
struct ListEl3_B : public DListEl<ListEl3> { };
struct ListEl4_A : public DListEl<ListEl4> { };
struct ListEl4_B : public DListEl<ListEl4> { };
struct ListEl5_A : public DListEl<ListEl5> { };
struct ListEl5_B : public DListEl<ListEl5> { };
struct ListEl6_A : public DListEl<ListEl6> { };
struct ListEl6_B : public DListEl<ListEl6> { };
struct ListEl1 : public ListEl1_A, public ListEl1_B { int i; };
struct ListEl2 : public ListEl2_A, public ListEl2_B { int i; };
struct ListEl3 : public ListEl3_A, public ListEl3_B { int i; };
struct ListEl4 : public ListEl4_A, public ListEl4_B { int i; };
struct ListEl5 : public ListEl5_A, public ListEl5_B { int i; };
struct ListEl6 : public ListEl6_A, public ListEl6_B { int i; };
typedef DListMel<ListEl1, ListEl1_A> List1;
typedef DListMel<ListEl2, ListEl2_A> List2;
typedef DListMel<ListEl3, ListEl3_A> List3;
typedef DListMel<ListEl4, ListEl4_A> List4;
typedef DListMel<ListEl5, ListEl5_A> List5;
typedef DListMel<ListEl6, ListEl6_A> List6;
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
	return 0;
}
