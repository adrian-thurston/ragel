/*
 * Copyright 2010-2012 Adrian Thurston <thurston@colm.net>
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

#ifndef _COLM_DEBUG_H
#define _COLM_DEBUG_H

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

extern const char *const colm_realm_names[REALMS];

#ifdef __cplusplus
}
#endif

#endif /* _COLM_DEBUG_H */

