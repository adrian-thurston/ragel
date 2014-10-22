/*
 *  Copyright 2010-2012 Adrian Thurston <thurston@complang.org>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

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

#define REALM_BYTECODE    0x00000001
#define REALM_PARSE       0x00000002
#define REALM_MATCH       0x00000004
#define REALM_COMPILE     0x00000008
#define REALM_POOL        0x00000010
#define REALM_PRINT       0x00000020
#define REALM_INPUT       0x00000040
#define REALM_SCAN        0x00000080

#define REALMS            32

extern const char *const colmRealmNames[REALMS];

#ifdef __cplusplus
}
#endif
