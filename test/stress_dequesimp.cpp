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

#include "dequesimp.h"
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <stdio.h>

#define STATBUF 100
#define MAXGET 40
#define MAXPUT 40

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
 * need only a single block and sometimes more than one. */
struct MyBlockSize
{
	int getBlockSize( ) const { return 10; }
};

typedef DequeSimp<char, MyBlockSize> VectDequeSimp;

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
	VectDequeSimp deque;
	while ( len > 0 || deque.length() > 0 ) {
		/* Select something to do. */
		if ( random() % 2 ) {
			if ( len > 0 ) {
				/* Find some random amount to put into the queue. */
				int putlen = random() % MAXPUT;
				if ( putlen > len )
					putlen = len;

				/* Randomly select how to put the data in. */
				if ( random() % 2 ) {
					/* All at once. */
					deque.append( src, putlen );
				}
				else {
					/* One at a time. */
					for ( int i = 0; i < putlen; i++ )
						deque.append( src[i] );
				}

				len -= putlen;
				src += putlen;
			}
		}
		else {
			if ( deque.length() > 0 ) {
				/* Find some random amout to take off the deque. */
				int getlen = random() % MAXGET;
				if ( getlen > deque.length() )
					getlen = deque.length();

				/* Randomly select a remove method. */
				switch ( random() % 3 ) {
				case 0: {
					/* Copy data out with an iterator, remove all at once. */
					VectDequeSimp::Iter i = deque.first(); 
					for ( int d = 0; d < getlen; d++, i++ )
						dst[d] = *i;

					/* Remove the items from the queue. */
					deque.removeFront( getlen );
					break;
				}
				case 1: {
					/* Use the remove that copies the data out for us. */
					deque.removeFront( dst, getlen );
					break;
				}
				case 2: {
					/* Peek and then remove. */
					deque.peekFront( dst, getlen );
					deque.removeFront( getlen );
					break;
				}}

				dst += getlen;
			}
		}
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
	 * deque to the desination. */
	char *dst = dstbuf, *src = srcbuf;
	int len = datalen;
	DequeSimp<char, MyBlockSize> deque;
	while ( len > 0 || deque.length() > 0 ) {
		/* Select something to do. */
		if ( random() % 2 ) {
			if ( len > 0 ) {
				/* Find some random amount to put into the queue. */
				int putlen = random() % MAXPUT;
				if ( putlen > len )
					putlen = len;
				char revbuf[MAXPUT];
				copyReverse( revbuf, src, putlen );

				/* Randomly select how to put the data in. */
				if ( random() % 2 ) {
					/* All at once. */
					deque.prepend( revbuf, putlen );
				}
				else {
					/* One at a time. */
					for ( int i = putlen-1; i >= 0; i-- )
						deque.prepend( revbuf[i] );
				}

				len -= putlen;
				src += putlen;
			}
		}
		else {
			if ( deque.length() > 0 ) {
				/* Find some random amout to take off the deque. */
				int getlen = random() % MAXGET;
				if ( getlen > deque.length() )
					getlen = deque.length();
				char revbuf[MAXGET];

				/* Randomly select between different ways to remove. */
				switch ( random() % 3 ) {
				case 0: {
					/* Copy data out with an iterator, remove all at once. */
					VectDequeSimp::Iter i = deque.last(); 
					for ( int d = getlen-1; d >= 0; d--, i-- )
						revbuf[d] = *i;

					/* Remove the items from the queue. */
					deque.removeEnd( getlen );
					break;
				}
				case 1: {
					/* Use the remove that copies the data out for us. */
					deque.removeEnd( revbuf, getlen );
					break;
				}
				case 2: {
					/* Peek and then remove. */
					deque.peekEnd( revbuf, getlen );
					deque.removeEnd( getlen );
					break;
				}}

				copyReverse( dst, revbuf, getlen );
				dst += getlen;
			}

			/* Check the amount coppied into dest so far. */
			int srclen = src - srcbuf;
			int dstlen = dst - dstbuf;
			int max = srclen < dstlen ? srclen : dstlen;
			if ( max > 0 && memcmp( srcbuf, dstbuf, max ) != 0 )
				assert( false );
		}

		count += 1;
	}

	/* Now assert that the copy preserved the contents. */
	if ( memcmp( dstbuf, srcbuf, datalen ) != 0 )
		assert( false );
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
