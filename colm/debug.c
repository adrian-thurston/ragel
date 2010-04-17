/*
 *  Copyright 2010 Adrian Thurston <thurston@complang.org>
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

#include "debug.h"
#include <stdarg.h>
#include <stdio.h>

long colmActiveRealm = 0;

int _debug( long realm, const char *fmt, ... )
{
	int result = 0;
	if ( colmActiveRealm & realm ) {
		va_list args;
		va_start( args, fmt );
		result = vfprintf( stderr, fmt, args );
		va_end( args );
	}

	return result;
}
