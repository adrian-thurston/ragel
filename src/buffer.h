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

#ifndef _COLM_BUFFER_H
#define _COLM_BUFFER_H

#define BUFFER_INITIAL_SIZE 4096

/* An automatically grown buffer for collecting tokens. Always reuses space;
 * never down resizes. */
struct Buffer
{
	Buffer()
	{
		data = (char*) malloc( BUFFER_INITIAL_SIZE );
		allocated = BUFFER_INITIAL_SIZE;
		length = 0;
	}
	~Buffer() { free(data); }

	void append( char p )
	{
		if ( length == allocated ) {
			allocated *= 2;
			data = (char*) realloc( data, allocated );
		}
		data[length++] = p;
	}
		
	void clear() { length = 0; }

	char *data;
	int allocated;
	int length;
};

#endif /* _COLM_BUFFER_H */

