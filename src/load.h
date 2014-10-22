/*
 *  Copyright 2013 Adrian Thurston <thurston@complang.org>
 */

#ifndef _LOAD_H
#define _LOAD_H

#include "ragel.h"

struct LoadRagel;
struct InputData;
struct HostLang;

LoadRagel *newLoadRagel( InputData &id, const HostLang *hostLang,
		MinimizeLevel minimizeLevel, MinimizeOpt minimizeOpt );
void loadRagel( LoadRagel *lr, const char *inputFileName );
void deleteLoadRagel( LoadRagel * );

#endif
