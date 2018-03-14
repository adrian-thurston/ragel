/*
 * Copyright 2001 Adrian Thurston <thurston@colm.net>
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
