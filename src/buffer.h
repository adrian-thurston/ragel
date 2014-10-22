/*
 *  Copyright 2003 Adrian Thurston <thurston@complang.org>
 */

#ifndef _BUFFER_H
#define _BUFFER_H

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

#endif /* _BUFFER_H */
