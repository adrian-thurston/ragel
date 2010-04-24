/*
 *  Copyright 2007-2010 Adrian Thurston <thurston@complang.org>
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

#ifndef _BYTECODE_H
#define _BYTECODE_H

#include "rtvector.h"
#include "config.h"
#include "pool.h"
#include "pdarun.h"
#include "map.h"

#include <iostream>

using std::cerr;
using std::endl;
using std::ostream;

#include "bytecode2.h"

typedef struct _Kid Kid;
typedef struct _Tree Tree;
typedef struct _ParseTree ParseTree;
typedef struct _ListEl ListEl;
typedef struct _MapEl MapEl;
typedef struct _List List;
typedef struct _Map Map;
typedef struct _Ref Ref;
typedef struct _Pointer Pointer;
typedef struct _Str Str;
typedef struct _Int Int;
typedef struct _PdaRun PdaRun;


void rcodeDownref( Program *prg, Tree **sp, Code *instr );

#include "tree.h"

Tree **stackAlloc();

/*
 * Runtime environment
 */


#endif
