/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _RAGEL_H
#define _RAGEL_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include "vector.h"
#include "config.h"
#include "common.h"

#define PROGNAME "ragel"

#define MAIN_MACHINE "main"

/* Target output style. */
enum CodeStyle
{
	GenBinaryLoop,
	GenBinaryExp,
	GenFlatLoop,
	GenFlatExp,
	GenSwitchLoop,
	GenSwitchExp,
	GenIpGoto
};

/* To what degree are machine minimized. */
enum MinimizeLevel {
	#ifdef TO_UPGRADE_CONDS
	MinimizeApprox,
	#endif
	#ifdef TO_UPGRADE_CONDS
	MinimizeStable,
	#endif
	MinimizePartition1,
	MinimizePartition2
};

enum MinimizeOpt {
	MinimizeNone,
	MinimizeEnd,
	MinimizeMostOps,
	MinimizeEveryOp
};

/* Target implementation */
enum RubyImplEnum
{
	MRI,
	Rubinius
};

/* Error reporting format. */
enum ErrorFormat {
	ErrorFormatGNU,
	ErrorFormatMSVC,
};

struct colm_location;

InputLoc makeInputLoc( const char *fileName, int line = 0, int col = 0 );
InputLoc makeInputLoc( const struct colm_location *loc );
std::ostream &operator<<( std::ostream &out, const InputLoc &loc );

/* Error reporting. */
std::ostream &error();
std::ostream &error( const InputLoc &loc ); 
std::ostream &warning( const InputLoc &loc ); 

void xmlEscapeHost( std::ostream &out, const char *data, long len );

/* IO filenames and stream. */
extern int gblErrorCount;

std::ostream &error();

extern const char mainMachine[];

#endif
