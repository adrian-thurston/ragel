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

#include "deque.h"
#include "dequeverify.h"

#include "vector.h"
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <stdio.h>

#define STATBUF 100
#define MAXVPUT 8
#define MAXVGET 8
#define BLOCKSIZE 2
#define VECTSIZE 5

using namespace std;
void processArgs( int argc, char** argv );
void expandTab( char *dst, char *src );

/* Do a byte-wise reversal of the contents of the buffer. */
char *reverseBuffer( char *buf, int len )
{
	int charsToSwitch = len / 2;
	char tmp, *front = buf, *end = buf + len - 1;

	while ( charsToSwitch-- ) {
		tmp = *front;
		*front = *end;
		*end = tmp;

		front++;
		end--;
	}

	return buf;
}

/* Reverse the contents of the buffer during the copy. */
void copyReverse( char *dst, char *src, int len )
{
	dst += len - 1;
	for ( char *slim = src + len; src < slim; dst--, src++ )
		*dst = *src;
}

/* Test with a small block size that will cause gets and puts to sometimes
 * need only a single block and sometimes more than one. Since we put five
 * bytes into a vector then the max amount of data that block can hold is 10
 * bytes. */
struct MyBlockSize
{
	int getBlockSize( ) const 
		{ return BLOCKSIZE; }
};

typedef DequeVer< Vector<char>, MyBlockSize > VectDeque;

void dequeStressForward()
{
	/* Pick some random file size between 1 byte and 64K and fill it with
	 * random data. */
	int datalen = 1 + (random() % 65536);
	char *srcbuf = new char[datalen];
	for ( int i = 0; i < datalen; i++ )
		srcbuf[i] = random() & 0xff;
	
	/* Set up a destination buffer of the same size. */
	char *dstbuf = new char[datalen];
	memset( dstbuf, 0, datalen );
	
	/* While there is srcbuf to transfer from the source to the deque or the
	 * deque to the desination. */
	char *dst = dstbuf, *src = srcbuf;
	int len = datalen;
	VectDeque deque;
	while ( len > 0 || deque.length() > 0 ) {
		/* Select either insert or remove. */
		if ( random() % 2 ) {
			/* Insert, do it only if there is some data in the source. */
			if ( len > 0 ) {
				/* Choose some random number of vector and fill them in. */
				int putvects = (random() % MAXVPUT) + 1;
				int highvect = 0;
				Vector<char> vects[putvects];
				for ( int v = 0; v < putvects && len > 0; v++ ) {
					int invect = VECTSIZE;
					if ( invect > len )
						invect = len;
					vects[v].setAs( src, invect );
					len -= invect;
					src += invect;
					highvect += 1;
				}

				/* Randomly choose what kind of insert to do. */
				if ( random() % 2 ) {
					/* All at once. */
					deque.append( vects, highvect );
				}
				else {
					/* One at a time. */
					for ( int v = 0; v < highvect; v++ )
						deque.append( vects[v] );
				}
			}
		}
		else {
			/* Remove, do it only if there are items in the queue. */
			if ( deque.length() > 0 ) {
				/* Take off a random number of vectors. */
				int getvects = (random() % MAXVPUT) + 1;
				if ( getvects > deque.length() )
					getvects = deque.length();

				/* Copy the vectors off the queue and put them into a local
				 * array. Randomly select between an iter, remove and peek. */
				Vector<char> vects[getvects];
				switch( random() % 3 ) {
				case 0: {
					/* Copy the data out with an iterator, then remove. */
					VectDeque::Iter i = deque.first(); 
					for ( int v = 0; v < getvects; v++, i++ )
						vects[v] = *i;

					/* Remove the items from the queue. */
					deque.removeFront( getvects );
					break;
				}
				case 1: {
					/* Use the remove that copies the data out for us. */
					deque.removeFront( vects, getvects );
					break;
				}
				case 2: {
					/* Peek and then remove the items. */
					deque.peekFront( vects, getvects );
					deque.removeFront( getvects );
					break;
				}}

				/* Copy the data to the output buffer. */
				for ( int v = 0; v < getvects; v++ ) {
					memcpy( dst, vects[v].data, vects[v].length() );
					dst += vects[v].length();
				}
			}
		}

		/* Test the deque, this is a cheap test so run 
		 * it on every iteration. */
		deque.verifyIntegrity();
	}

	/* Now assert that the copy preserved the contents. */
	assert( memcmp( dstbuf, srcbuf, datalen ) == 0 );

	delete[] srcbuf;
	delete[] dstbuf;
}

void dequeStressReverse()
{
	int count = 0;

	/* Pick some random file size between 1 byte and 64K and fill it with
	 * random data. */
	int datalen = 1 + (random() % 65536);
	char *srcbuf = new char[datalen];
	for ( int i = 0; i < datalen; i++ )
		srcbuf[i] = random() & 0xff;
	
	/* Set up a destination buffer of the same size. */
	char *dstbuf = new char[datalen];
	memset( dstbuf, 0, datalen );
	
	/* While there is srcbuf to transfer from the source to the deque or the
	 * deque to the destination. */
	char *dst = dstbuf, *src = srcbuf;
	int len = datalen;
	VectDeque deque;

	while ( len > 0 || deque.length() > 0 ) {
		/* Select either insert or remove. */
		if ( random() % 2 ) {
			/* Insert, do it only if there is some data in the source. */
			if ( len > 0 ) {
				/* Choose some random number of vector and fill them in. */
				int putvects = (random() % MAXVPUT) + 1;
				int lowvect = putvects;
				Vector<char> vects[putvects];
				for ( int v = putvects-1; v >= 0 && len > 0; v-- ) {
					int invect = VECTSIZE;
					if ( invect > len )
						invect = len;
					char revbuf[VECTSIZE];
					copyReverse( revbuf, src, invect );
					vects[v].setAs( revbuf, invect );
					len -= invect;
					src += invect;
					lowvect -= 1;
				}

				/* Randomly choose what kind of insert to do. */
				if ( random() % 2 ) {
					/* All at once. */
					deque.prepend( vects+lowvect, putvects-lowvect );
				}
				else {
					/* Prepend One at a time. */
					for ( int v = putvects-1; v >= lowvect; v-- )
						deque.prepend( vects[v] );
				}
			}
		}
		else {
			/* Remove, do it only if there are items in the queue. */
			if ( deque.length() > 0 ) {
				/* Take off a random number of vectors. */
				int getvects = (random() % MAXVPUT) + 1;
				if ( getvects > deque.length() )
					getvects = deque.length();

				/* Randomly select between taking the items off with an
				 * iterator and with the remove that does it for us. */
				Vector<char> vects[getvects];
				switch ( random() % 3 ) {
				case 0: {
					/* Copy the vectors off the queue and put them into a local
					 * array. */
					VectDeque::Iter i = deque.last(); 
					for ( int v = getvects-1; v >= 0; v--, i-- )
						vects[v] = *i;

					/* Remove the items from the queue. */
					deque.removeEnd( getvects );
					break;
				}
				case 1: {
					/* Use the remove that copies the data out for us. */
					deque.removeEnd( vects, getvects );
					break;
				}
				case 2: {
					/* Red the data off then remove it. */
					deque.peekEnd( vects, getvects );
					deque.removeEnd( getvects );
					break;
				}}

				/* Copy the data to the output buffer. */
				for ( int v = getvects-1; v >= 0; v-- ) {
					copyReverse( dst, vects[v].data, vects[v].length() );
					dst += vects[v].length();
				}
			}
		}

		/* Test the deque, this is a cheap test so run 
		 * it on every iteration. */
		deque.verifyIntegrity();
		count += 1;
	}

	/* Now assert that the copy preserved the contents. */
	assert( memcmp( dstbuf, srcbuf, datalen ) == 0 );

	delete[] srcbuf;
	delete[] dstbuf;
}

void printRound( )
{
	static int round = 0;
	static char buf[STATBUF] = { 0 };
	char tmpBuf[STATBUF];

	memset( buf, '\b', strlen(buf) );
	cout << buf;
	sprintf( tmpBuf, "round:\t%7i", round );
	expandTab( buf, tmpBuf );
	cout << buf;
	cout.flush();
	round += 1;
}

int main( int argc, char **argv )
{
	processArgs( argc, argv );
	srandom( time(0) );

	while ( true ) {
		/* Select between the various tests. */
		if ( random() % 2 ) 
			dequeStressForward();
		else
			dequeStressReverse();
		printRound();
	}
	return 0;
}
