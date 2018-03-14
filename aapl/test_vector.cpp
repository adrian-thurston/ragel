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
#include <assert.h>

#include "resize.h"
#include "vector.h"

using namespace std;

template class Vector< int, ResizeExpn >;
template class Vector< int, ResizeLin >;
template class Vector< int, ResizeConst >;
template class Vector< int, ResizeRunTime >;
template class Vector< int, ResizeCtLin<0> >;

void testVector1()
{
	int pos, len;
	char c, string[200];
	Vector<char, ResizeLin> v;

	while( scanf("%c", &c) == 1 )
	{
		switch (c) {
			case 'a':
				/* Get the string to append. */
				fgets(string, 200, stdin);

				/* hack off the newline. */
				len = strlen(string) - 1;
				string[len] = 0;

				v.append(string, len);
				break;

			case 'p':
				/* Get the string to prepend. */
				fgets(string, 200, stdin);

				/* hack off the newline. */
				len = strlen(string) - 1;
				string[len] = 0;

				v.prepend(string, len);
				break;

			case 's':
				/* Get the string to set. */
				fgets(string, 200, stdin);

				/* hack off the newline. */
				len = strlen(string) - 1;
				string[len] = 0;

				v.setAs(string, len);
				break;

			case 'r':
				/* Get the pos */
				scanf("%i ", &pos);

				/* Get the string to replace. */
				fgets(string, 200, stdin);

				/* hack off the newline. */
				len = strlen(string) - 1;
				string[len] = 0;

				v.replace(pos, string, len);
				break;
				
			case 'i':
				/* Get the pos */
				scanf("%i ", &pos);

				/* Get the string to insert. */
				fgets(string, 200, stdin);

				/* hack off the newline. */
				len = strlen(string) - 1;
				string[len] = 0;

				v.insert(pos, string, len);
				break;
				
			case 'd':
				/* Get the length to remove. */
				scanf("%i", &pos);
				scanf("%i", &len);
				v.remove(pos, len);
				break;

			case 'w':
				printf("L: %li  AL: %li\n", v.tabLen, v.allocLen);
				fwrite(v.data, 1, v.tabLen, stdout);
				fputc('\n', stdout);
				break;

			case 'q':
				goto end;

		}
	}

end:
	fwrite(v.data, 1, v.tabLen, stdout);
	fputc('\n', stdout);
}


/* Globals that the constructors and destructor count with. */
int defaultCount = 0;
int copyCount = 0;
int destructorCount = 0;

/* Reset the globals for the next test. */
void reset()
{
	defaultCount = 0;
	copyCount = 0;
	destructorCount = 0;
}

/* An assert that resets the globals and does an assert. */
#define assert_reset(test) assert(test); reset();

/* Data structure whos constructors and destructor count callings. */
struct theData
{
	theData() : i(0) { defaultCount++; }
	theData(const theData &d) : i(1) { copyCount++; }
	~theData() { destructorCount++; }
	int i;
};

void testDelete()
{
	/* Initialization. */
	Vector< theData, ResizeLin > v1;
	v1.setAsNew(10);
	assert_reset( defaultCount == 10 );

	/* Delete some. */
	v1.remove( 5, 3 );
	assert_reset( destructorCount == 3 );

	/* Delete some more. */
	v1.remove( 0, 4 );
	assert_reset( destructorCount == 4 );

	/* Test tabLen. */
	assert_reset( v1.tabLen == 3 );
}

void testInsert()
{
	/* Simple Construct calls. */
	theData sampleData[10];
	assert_reset( defaultCount == 10 );
	
	/* Initialization. */
	Vector< theData, ResizeLin > v1;
	v1.setAsNew(6);
	assert_reset( defaultCount == 6 );

	/* Copy Constructor. */
	Vector< theData, ResizeLin > v2(v1);
	assert_reset( copyCount == 6 );

	/* Insert, some shifted over. */
	v1.insert(1, sampleData, 4);
	assert_reset( v1.tabLen == 10 && destructorCount == 0 && copyCount == 4 );

	/* Insert, none shifted over. */
	v1.insert(10, sampleData, 4);
	assert_reset( defaultCount == 0 && copyCount == 4 );

	/* Insert some new items. */
	v1.insertNew(0, 10);
	assert_reset( v1.tabLen == 24 && defaultCount == 10 && copyCount == 0 );

	/* Insert some duplicates. */
	v1.insertDup(-1, theData(), 6);
	assert_reset( v1.tabLen == 30 && defaultCount == 1 && copyCount == 6 );
}

void testSetAs()
{
	/* Simple Construct calls. */
	theData sampleData[10];
	assert_reset( defaultCount == 10 );
	
	/* Initialization. */
	Vector< theData, ResizeLin > v1;
	v1.setAsNew(6);
	assert_reset( defaultCount == 6 );

	v1.setAs( sampleData, 10 );
	assert_reset( destructorCount == 6 && copyCount == 10 && defaultCount == 0 );
}


void testReplace()
{
	/* Simple Construct calls. */
	theData sampleData[10];
	assert_reset( defaultCount == 10 );
	
	/* Initialization. */
	Vector< theData, ResizeLin > v1;
	v1.setAsNew(4);
	assert_reset( defaultCount == 4 );

	/* Copy Constructor. */
	Vector< theData, ResizeLin > v2(v1);
	assert_reset( copyCount == 4 );

	/* Replace, some remove, some new. */
	v1.replace(1, sampleData, 4);
	assert_reset( destructorCount == 3 && copyCount == 4 );

	/* Replace, all remove. */
	v1.replace(0, sampleData, 5);
	assert_reset( destructorCount == 5 && copyCount == 5 );

	/* Replace, all new. */
	v1.replace(5, sampleData, 2);
	assert_reset( destructorCount == 0 && copyCount == 2 );

	/* Overwrite without going to the end. */
	v1.replace(0, sampleData, 1);
	assert_reset( destructorCount == 1 && defaultCount == 0 && copyCount == 1 );

	/* Ensure tabLen is good. */
	assert_reset( v1.tabLen == 7 );
}


void testWithValues()
{
	Vector< int, ResizeLin > v1;
	Vector< int, ResizeLin > v2;

	int i = 1;
	v1.append( i );
	v2.setAs(v1);
	i = 2;
	v1.insert( 0, i );
	assert_reset( v1.data[0] == 2 && v2.data[0] == 1 );

}

void testOperators1()
{
	Vector<int, ResizeLin> v;
	v.append( 10 );
	v.append( 11 );

	cout << v.size() << endl;
	cout << v.data[0] << endl;
	cout << v[1] << endl;

	Vector<int, ResizeLin>::Iter it;
	for ( it = v; it.lte(); it++)
		cout << *it << endl;

	it = v.last();
	for ( it = v; it.gtb(); it-- )
		cout << *it << endl;
}

void testOperators2()
{
	Vector< Vector<int> > vvi;
	vvi.setAsNew(1);
	Vector< Vector<int> >::Iter it;
	for ( it = vvi; it.lte(); it++ )
		cout << it->size() << endl;
}

void testCopying()
{
	Vector<int> v1;
	v1.append(1);
	v1.append(2);
	v1.append(3);

	Vector<int> v2;
	v2.append(4);
	v2 = v1;
	cout << v2.size() << endl;
}

void testBounds()
{
	Vector<int, ResizeLin> v;
	v.setAsNew(1);
	v.setAsNew(0);
	return;
}

typedef Vector<int> Vect;

void testIterator()
{
	Vect vect;

	cout << "Iterator Default Constructor" << endl;

	/* Default constructed iterator. */
	Vect::Iter defIt;
	cout << defIt.lte() << " " << defIt.end() << " " << defIt.gtb() << " " << 
			defIt.beg() << " " << defIt.first() << " " << defIt.last() << endl;

	cout << "Zero Items" << endl;

	/* Iterator constructed from empty vect. */
	Vect::Iter i1(vect);
	cout << i1.lte() << " " << i1.end() << " " << i1.gtb() << " " << 
			i1.beg() << " " << i1.first() << " " << i1.last() << endl;
	Vect::Iter i2(vect.first());
	cout << i2.lte() << " " << i2.end() << " " << i2.gtb() << " " << 
			i2.beg() << " " << i2.first() << " " << i2.last() << endl;
	Vect::Iter i3(vect.last());
	cout << i3.lte() << " " << i3.end() << " " << i3.gtb() << " " << 
			i3.beg() << " " << i3.first() << " " << i3.last() << endl;
	
	/* Iterator assigned from an empty vect. */
	i1 = vect;
	cout << i1.lte() << " " << i1.end() << " " << i1.gtb() << " " << 
			i1.beg() << " " << i1.first() << " " << i1.last() << endl;
	i2 = vect.first();
	cout << i2.lte() << " " << i2.end() << " " << i2.gtb() << " " << 
			i2.beg() << " " << i2.first() << " " << i2.last() << endl;
	i3 = vect.last();
	cout << i3.lte() << " " << i3.end() << " " << i3.gtb() << " " << 
			i3.beg() << " " << i3.first() << " " << i3.last() << endl;

	cout << "One Item" << endl;
	vect.append( 1 );

	/* Iterator constructed from an a vect with one item. */
	Vect::Iter i4(vect);
	cout << i4.lte() << " " << i4.end() << " " << i4.gtb() << " " << 
			i4.beg() << " " << i4.first() << " " << i4.last() << endl;
	Vect::Iter i5(vect.first());
	cout << i5.lte() << " " << i5.end() << " " << i5.gtb() << " " << 
			i5.beg() << " " << i5.first() << " " << i5.last() << endl;
	Vect::Iter i6(vect.last());
	cout << i6.lte() << " " << i6.end() << " " << i6.gtb() << " " << 
			i6.beg() << " " << i6.first() << " " << i6.last() << endl;

	/* Iterator assigned from an a vect with one item. */
	i4 = vect;
	cout << i4.lte() << " " << i4.end() << " " << i4.gtb() << " " << 
			i4.beg() << " " << i4.first() << " " << i4.last() << endl;
	i5 = vect.first();
	cout << i5.lte() << " " << i5.end() << " " << i5.gtb() << " " << 
			i5.beg() << " " << i5.first() << " " << i5.last() << endl;
	i6 = vect.last();
	cout << i6.lte() << " " << i6.end() << " " << i6.gtb() << " " << 
			i6.beg() << " " << i6.first() << " " << i6.last() << endl;

	cout << "Two Items" << endl;
	vect.append( 2 );

	/* Iterator constructed from an a vect with two items. */
	Vect::Iter i7(vect);
	cout << i7.lte() << " " << i7.end() << " " << i7.gtb() << " " << 
			i7.beg() << " " << i7.first() << " " << i7.last() << endl;
	Vect::Iter i8(vect.first());
	cout << i8.lte() << " " << i8.end() << " " << i8.gtb() << " " << 
			i8.beg() << " " << i8.first() << " " << i8.last() << endl;
	Vect::Iter i9(vect.last());
	cout << i9.lte() << " " << i9.end() << " " << i9.gtb() << " " << 
			i9.beg() << " " << i9.first() << " " << i9.last() << endl;

	/* Iterator assigned from an a vect with two items. */
	i7 = vect;
	cout << i7.lte() << " " << i7.end() << " " << i7.gtb() << " " << 
			i7.beg() << " " << i7.first() << " " << i7.last() << endl;
	i8 = vect.first();
	cout << i8.lte() << " " << i8.end() << " " << i8.gtb() << " " << 
			i8.beg() << " " << i8.first() << " " << i8.last() << endl;
	i9 = vect.last();
	cout << i9.lte() << " " << i9.end() << " " << i9.gtb() << " " << 
			i9.beg() << " " << i9.first() << " " << i9.last() << endl;

	vect.empty();
	vect.append( 1 );
	vect.append( 2 );
	vect.append( 3 );
	vect.append( 4 );

	cout << "test1" << endl;
	Vect::Iter it1 = vect;
	for ( ; it1.lte(); it1++ )
		cout << *it1 << endl;

	cout << "test2" << endl;
	Vect::Iter it2 = vect.first();
	for ( ; it2.lte(); it2++ )
		cout << *it2 << endl;

	cout << "test3" << endl;
	Vect::Iter it3 = vect.last();
	for ( ; it3.gtb(); it3-- )
		cout << *it3 << endl;

	cout << "test4" << endl;
	it1 = vect;
	for ( ; !it1.end(); it1++ )
		cout << *it1 << endl;

	cout << "test5" << endl;
	it2 = vect.first();
	for ( ; !it2.end(); it2++ )
		cout << *it2 << endl;

	cout << "test6" << endl;
	it3 = vect.last();
	for ( ; !it3.beg(); it3-- )
		cout << *it3 << endl;
}

void testConstructor()
{
	reset();
	theData someData;
	Vector<theData> v1( someData );
	v1.append( theData() );
	Vector<theData> v2( v1.data, v1.length() );

	cout << "testConstructor" << endl;
	assert( destructorCount == 1 && defaultCount == 2 && copyCount == 4 );
}

int main()
{
	testReplace();
	testInsert();
	testSetAs();
	testDelete();
	testWithValues();
	testOperators1();
	testOperators2();
	testCopying();
	testBounds();
	testIterator();
	testConstructor();
	return 0;
}
