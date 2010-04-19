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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef COLM_LOG
#define debug( realm, fmt, ...) _debug( realm, fmt, __VA_ARGS__ )
#else
#define debug( realm, fmt, ...) 
#endif

int _debug( long realm, const char *fmt, ... );
extern long colmActiveRealm;

#define REALM_BYTECODE    0x00000001
#define REALM_PARSE       0x00000002
#define REALM_MATCH       0x00000004
#define REALM_COMPILE     0x00000008

#ifdef __cplusplus
}
#endif
