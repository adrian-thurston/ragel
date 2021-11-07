#ifndef _NRAGEL_H
#define _NRAGEL_H

#include <libfsm/common.h>
#include <libfsm/ragel.h>

HostType *findAlphType( const HostLang *hostLang, const char *s1 );
HostType *findAlphType( const HostLang *hostLang, const char *s1, const char *s2 );
HostType *findAlphTypeInternal( const HostLang *hostLang, const char *s1 );

#endif
