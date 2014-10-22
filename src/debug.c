/*
 *  Copyright 2010-2012 Adrian Thurston <thurston@complang.org>
 */

#include <colm/debug.h>
#include <colm/program.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

const char *const colmRealmNames[REALMS] =
	{
		"BYTECODE",
		"PARSE",
		"MATCH",
		"COMPILE",
		"POOL",
		"PRINT",
		"INPUT",
		"SCAN",
	};

int _debug( struct colm_program *prg, long realm, const char *fmt, ... )
{
	int result = 0;
	if ( prg->activeRealm & realm ) {
		/* Compute the index by shifting. */
		int ind = 0;
		while ( (realm & 0x1) != 0x1 ) {
			realm >>= 1;
			ind += 1;
		}

		fprintf( stderr, "%s: ", colmRealmNames[ind] );
		va_list args;
		va_start( args, fmt );
		result = vfprintf( stderr, fmt, args );
		va_end( args );
	}

	return result;
}

void fatal( const char *fmt, ... )
{
	va_list args;
	fprintf( stderr, "fatal: " );
	va_start( args, fmt );
	vfprintf( stderr, fmt, args );
	va_end( args );
	exit(1);
}

void message( const char *fmt, ... )
{
	va_list args;
	fprintf( stderr, "message: " );
	va_start( args, fmt );
	vfprintf( stderr, fmt, args );
	va_end( args );
}
