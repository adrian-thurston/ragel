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
