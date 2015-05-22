/*
 *  Copyright 2010-2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <colm/debug.h>
#include <colm/program.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

const char *const colm_realm_names[REALMS] =
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
	if ( prg->active_realm & realm ) {
		/* Compute the index by shifting. */
		int ind = 0;
		while ( (realm & 0x1) != 0x1 ) {
			realm >>= 1;
			ind += 1;
		}

		fprintf( stderr, "%s: ", colm_realm_names[ind] );
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
