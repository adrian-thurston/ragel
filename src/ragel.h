/*
 * Copyright 2001-2007 Adrian Thurston <thurston@colm.net>
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

extern ErrorFormat errorFormat;


struct colm_location;

InputLoc makeInputLoc( const char *fileName, int line = 0, int col = 0 );
InputLoc makeInputLoc( const struct colm_location *loc );
std::ostream &operator<<( std::ostream &out, const InputLoc &loc );

void xmlEscapeHost( std::ostream &out, const char *data, long len );


using std::endl;

extern const char mainMachine[];

struct AbortCompile
{
	AbortCompile( int code )
		: code(code) {}

	int code;
};

#endif
