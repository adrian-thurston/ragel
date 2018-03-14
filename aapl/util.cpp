/*
 * Copyright 2002 Adrian Thurston <thurston@colm.net>
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
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#define TAB_WIDTH 10

using namespace std;

void handleAlarm( int )
{
	cout << endl;
	exit(0);
}

void processArgs( int argc, char** argv )
{
	if ( argc > 1 ) {
		int secs = atoi( argv[1] );
		if ( secs > 0 ) {
			signal( SIGALRM, &handleAlarm );
			alarm( secs );
		}
	}
}

/* Expand the tabs in a buffer a buffer. */
void expandTab( char *dst, const char *src )
{
	char *pd = dst;
	for ( ; *src != 0; src++ ) {
		if ( *src != '\t' )
			*pd++ = *src;
		else {
			int n = TAB_WIDTH - ((pd - dst)%TAB_WIDTH);
			memset( pd, ' ', n );
			pd += n;
		}
	}
	*pd = 0;
}


