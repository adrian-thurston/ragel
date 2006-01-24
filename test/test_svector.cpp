/*
 *  Copyright 2001, 2003 Adrian Thurston <thurston@cs.queensu.ca>
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
#include <assert.h>
#include <time.h>

#include "resize.h"
#include "vector.h"
#include "svector.h"
#include "compare.h"

using namespace std;

template class SVector< int, ResizeExpn >;
template class SVector< int, ResizeLin >;
template class SVector< int, ResizeConst >;
template class SVector< int, ResizeRunTime >;
template class SVector< int, ResizeCtLin<0> >;

template struct CmpSTable< int, CmpOrd<int> >;

void testVector1()
{
	int pos, len;
	char c, string[200];
	SVector<char, ResizeLin> v;

	while( scanf("%c", &c) == 1 ) {
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
				if ( v.data == 0 ) {
					printf("L: 0  AL: 0\n");
					fputc('\n', stdout);
				}
				else {
					STabHead *head = ((STabHead*)v.data) - 1;
					printf("L: %li  AL: %li\n", head->tabLen, head->allocLen);
					fwrite(v.data, 1, head->tabLen, stdout);
					fputc('\n', stdout);
				}
				break;

			case 'q':
				goto end;

		}
	}

end:
	fwrite(v.data, 1, v.length(), stdout);
	fputc('\n', stdout);
}

/* Globals that the constructors and destructor count with. */
int defaultCount = 0;
int nonDefCount = 0;
int copyCount = 0;
int destructorCount = 0;

/* Reset the globals for the next test. */
void reset()
{
	defaultCount = 0;
	nonDefCount = 0;
	copyCount = 0;
	destructorCount = 0;
}

/* An assert that resets the globals and does an assert. */
#define assert_reset(test) assert(test); reset();

/* Data structure whos constructors and destructor count callings. */
struct TheData
{
	TheData() : i(0) { defaultCount++; }
	TheData(int i) : i(i) { nonDefCount++; }
	TheData(const TheData &d) : i(1) { copyCount++; }
	~TheData() { destructorCount++; }
	int i;
};

bool operator==(const TheData &td1, const TheData &td2)
	{ return td1.i == td2.i; }

void testDelete()
{
	/* Initialization. */
	SVector< TheData, ResizeLin > v1;
	v1.setAsNew(10);
	assert_reset( defaultCount == 10 );

	/* Delete some. */
	v1.remove( 5, 3 );
	assert_reset( destructorCount == 3 );

	/* Delete some more. */
	v1.remove( 0, 4 );
	assert_reset( destructorCount == 4 );

	/* Test length. */
	assert_reset( v1.length() == 3 );
}

void testInsert()
{
	/* Simple Construct calls. */
	TheData sampleData[10];
	assert_reset( defaultCount == 10 );
	
	/* Initialization. */
	SVector< TheData, ResizeLin > v1;
	v1.setAsNew(6);
	assert_reset( defaultCount == 6 );

	/* Copy Constructor. */
	SVector< TheData, ResizeLin > v2(v1);
	assert_reset( copyCount == 0 );

	/* Insert, some shifted over. */
	v1.insert(1, sampleData, 4);
	assert_reset( v1.length() == 10 && destructorCount == 0 && copyCount == 10 );

	/* Insert, none shifted over. */
	v1.insert(10, sampleData, 4);
	assert_reset( defaultCount == 0 && copyCount == 4 );

	/* Insert some new items. */
	v1.insertNew(0, 10);
	assert_reset( v1.length() == 24 && defaultCount == 10 && copyCount == 0 );

	/* Insert some duplicates. */
	v1.insertDup(-1, TheData(), 6);
	assert_reset( v1.length() == 30 && defaultCount == 1 && copyCount == 6 );
}

void testSetAs()
{
	/* Simple Construct calls. */
	TheData sampleData[10];
	assert_reset( defaultCount == 10 );
	
	/* Initialization. */
	SVector< TheData, ResizeLin > v1;
	v1.setAsNew(6);
	assert_reset( defaultCount == 6 );

	v1.setAs( sampleData, 10 );
	assert_reset( destructorCount == 6 && copyCount == 10 && defaultCount == 0 );
}


void testReplace()
{
	/* Simple Construct calls. */
	TheData sampleData[10];
	assert_reset( defaultCount == 10 );
	
	/* Initialization. */
	SVector< TheData, ResizeLin > v1;
	v1.setAsNew(4);
	assert_reset( defaultCount == 4 );

	/* Copy Constructor. */
	SVector< TheData, ResizeLin > v2(v1);
	assert_reset( copyCount == 0 );

	/* Replace, some remove, some new. */
	v1.replace(1, sampleData, 4);
	assert_reset( destructorCount == 0 && copyCount == 5 );

	/* Replace, all remove. */
	v1.replace(0, sampleData, 5);
	assert_reset( destructorCount == 5 && copyCount == 5 );

	/* Replace, all new. */
	v1.replace(5, sampleData, 2);
	assert_reset( destructorCount == 0 && copyCount == 2 );

	/* Overwrite without going to the end. */
	v1.replace(0, sampleData, 1);
	assert_reset( destructorCount == 1 && defaultCount == 0 && copyCount == 1 );

	/* Ensure length is good. */
	assert_reset( v1.length() == 7 );
}

void testWithValues()
{
	SVector< int, ResizeLin > v1;
	SVector< int, ResizeLin > v2;

	int i = 1;
	v1.append( i );
	v2.setAs(v1);
	i = 2;
	v1.insert( 0, i );
	assert_reset( v1.data[0] == 2 && v2.data[0] == 1 );

}

void testOperators1()
{
	SVector<int, ResizeLin> v;
	v.append( 10 );
	v.append( 11 );

	printf("%li\n", v.size());
	printf("%i\n", v[0]);
	printf("%i\n", v[1]);

	SVector<int, ResizeLin>::Iter it;
	for ( it = v; it.lte(); it++ )
		cout << *it << endl;

	SVector<int, ResizeLin>::Iter itr;
	for ( itr = v; itr.gtb(); itr-- )
		cout << *itr << endl;
}

void testOperators2()
{
	SVector< SVector<int> > vvi;
	vvi.setAsNew(1);
	SVector< SVector<int> >::Iter it;
	for ( it = vvi; it.lte(); it++ )
		cout << it->size() << endl;
}

void testCopying()
{
	SVector<int> v1;
	v1.append(1);
	v1.append(2);
	v1.append(3);

	SVector<int> v2;
	v2.append(4);
	v2 = v1;
	cout << v2.size() << endl;
}

/* Data structure whos constructors and destructor count callings. */
struct NoisyData
{
	NoisyData() : i(0) { cout << __PRETTY_FUNCTION__ << endl; }
	NoisyData(int i) : i(i) { cout << __PRETTY_FUNCTION__ << endl; }
	NoisyData(const NoisyData &d) : i(d.i) { cout << __PRETTY_FUNCTION__ << endl; }
	~NoisyData() { cout << __PRETTY_FUNCTION__ << endl; }
	int i;
};

template class SVector<NoisyData>;

int testShared()
{
	cout << sizeof(SVector<NoisyData>) << endl;
	SVector<NoisyData> v1;
	v1.setAsNew(2);
	SVector<NoisyData> v2;
	SVector<NoisyData> v3;

	v2.insert( 0, NoisyData(1) );
	v2.insert( 0, NoisyData(2) );
	v2.insert( 0, NoisyData(3) );

	v3 = v2;
	v3.replaceDup( 1, NoisyData(4), 4 );

	for ( int i = 0; i < v3.length(); i++ )
		cout << v3.data[i].i << endl;

	return 0;
}

#define LEN_THRESH 1000
#define STATS_PERIOD 211
#define OUTBUFSIZE 1000
#define NUM_ROUNDS 1e9
int curRound = 0;

void expandTab( char *dst, char *src );


/* Print a statistics header. */
void printHeader()
{
	char buf[OUTBUFSIZE];
	expandTab( buf, "round\tlength\n" );
	fputs(buf, stdout);
}


/* Replace the current stats line with new stats. For one vect. */
void printStats( Vector<TheData> &v1 )
{
	/* Print stats. */
	char buf1[OUTBUFSIZE];
	char buf2[OUTBUFSIZE];

	memset( buf1, '\b', OUTBUFSIZE );
	fwrite( buf1, 1, OUTBUFSIZE, stdout );
	sprintf( buf1, "%i\t%li\t", curRound, v1.length() );
	expandTab( buf2, buf1 );
	fputs( buf2, stdout );
	fflush( stdout);
}

Vector<TheData> v1;
SVector<TheData> v2;
typedef SVector<int> SVect;

void testIterator()
{
	SVect vect;

	cout << "Iterator Default Constructor" << endl;

	/* Default constructed iterator. */
	SVect::Iter defIt;
	cout << defIt.lte() << " " << defIt.end() << " " << defIt.gtb() << " " << 
			defIt.beg() << " " << defIt.first() << " " << defIt.last() << endl;

	cout << "Zero Items" << endl;

	/* Iterator constructed from empty vect. */
	SVect::Iter i1(vect);
	cout << i1.lte() << " " << i1.end() << " " << i1.gtb() << " " << 
			i1.beg() << " " << i1.first() << " " << i1.last() << endl;
	SVect::Iter i2(vect.first());
	cout << i2.lte() << " " << i2.end() << " " << i2.gtb() << " " << 
			i2.beg() << " " << i2.first() << " " << i2.last() << endl;
	SVect::Iter i3(vect.last());
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
	SVect::Iter i4(vect);
	cout << i4.lte() << " " << i4.end() << " " << i4.gtb() << " " << 
			i4.beg() << " " << i4.first() << " " << i4.last() << endl;
	SVect::Iter i5(vect.first());
	cout << i5.lte() << " " << i5.end() << " " << i5.gtb() << " " << 
			i5.beg() << " " << i5.first() << " " << i5.last() << endl;
	SVect::Iter i6(vect.last());
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
	SVect::Iter i7(vect);
	cout << i7.lte() << " " << i7.end() << " " << i7.gtb() << " " << 
			i7.beg() << " " << i7.first() << " " << i7.last() << endl;
	SVect::Iter i8(vect.first());
	cout << i8.lte() << " " << i8.end() << " " << i8.gtb() << " " << 
			i8.beg() << " " << i8.first() << " " << i8.last() << endl;
	SVect::Iter i9(vect.last());
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
	SVect::Iter it1 = vect;
	for ( ; it1.lte(); it1++ )
		cout << *it1 << endl;

	cout << "test2" << endl;
	SVect::Iter it2 = vect.first();
	for ( ; it2.lte(); it2++ )
		cout << *it2 << endl;

	cout << "test3" << endl;
	SVect::Iter it3 = vect.last();
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
	testShared();
	testIterator();
	return 0;
}
