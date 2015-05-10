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

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "colm.h"

void fatal( const char *fmt, ... );

#ifdef DEBUG
#define debug( prg, realm, ... ) _debug( prg, realm, __VA_ARGS__ )
#define check_realm( realm ) _check_realm( realm )
#else
#define debug( prg, realm, ... ) 
#define check_realm( realm ) 
#endif

struct colm_program;

int _debug( struct colm_program *prg, long realm, const char *fmt, ... );

void message( const char *fmt, ... );

#define REALM_BYTECODE    COLM_DBG_BYTECODE
#define REALM_PARSE       COLM_DBG_PARSE
#define REALM_MATCH       COLM_DBG_MATCH
#define REALM_COMPILE     COLM_DBG_COMPILE
#define REALM_POOL        COLM_DBG_POOL
#define REALM_PRINT       COLM_DBG_PRINT
#define REALM_INPUT       COLM_DBG_INPUT
#define REALM_SCAN        COLM_DBG_SCAN

#define REALMS            32

extern const char *const colmRealmNames[REALMS];

#ifdef __cplusplus
}
#endif
