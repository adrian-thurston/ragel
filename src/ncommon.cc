/*
 * Copyright 2006-2018 Adrian Thurston <thurston@colm.net>
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

#include "stdlib.h"
#include <string.h>
#include <assert.h>
#include <libfsm/common.h>
#include <libfsm/ragel.h>

/*
 * C
 */

const char *defaultOutFnC( const char *inputFileName )
{
	const char *ext = findFileExtension( inputFileName );
	if ( ext != 0 && strcmp( ext, ".rh" ) == 0 )
		return fileNameFromStem( inputFileName, ".h" );
	else
		return fileNameFromStem( inputFileName, ".c" );
}

HostType hostTypesC[] =
{
	{ "char",      0,      "char",    (CHAR_MIN != 0), true,  false,  SCHAR_MIN, SCHAR_MAX,  0, UCHAR_MAX,  sizeof(char) },
	{ "signed",   "char",  "char",    true,            true,  false,  SCHAR_MIN, SCHAR_MAX,  0, 0,          sizeof(signed char) },
	{ "unsigned", "char",  "uchar",   false,           true,  false,  0, 0,                  0, UCHAR_MAX,  sizeof(unsigned char) },
	{ "short",     0,      "short",   true,            true,  false,  SHRT_MIN,  SHRT_MAX,   0, 0,          sizeof(short) },
	{ "signed",   "short", "short",   true,            true,  false,  SHRT_MIN,  SHRT_MAX,   0, 0,          sizeof(signed short) },
	{ "unsigned", "short", "ushort",  false,           true,  false,  0, 0,                  0, USHRT_MAX,  sizeof(unsigned short) },
	{ "int",       0,      "int",     true,            true,  false,  INT_MIN,   INT_MAX,    0, 0,          sizeof(int) },
	{ "signed",   "int",   "int",     true,            true,  false,  INT_MIN,   INT_MAX,    0, 0,          sizeof(signed int) },
	{ "unsigned", "int",   "uint",    false,           true,  false,  0, 0,                  0, UINT_MAX,   sizeof(unsigned int) },
	{ "long",      0,      "long",    true,            true,  false,  LONG_MIN,  LONG_MAX,   0, 0,          sizeof(long) },
	{ "signed",   "long",  "long",    true,            true,  false,  LONG_MIN,  LONG_MAX,   0, 0,          sizeof(signed long) },
	{ "unsigned", "long",  "ulong",   false,           true,  false,  0, 0,                  0, ULONG_MAX,  sizeof(unsigned long) },
};

extern "C" const HostLang hostLangC = {
	hostTypesC,
	12,
	0,
	true,
	false, /* loopLabels */
	Direct,
	GotoFeature,
	&makeCodeGen,
	&defaultOutFnC,
	&genLineDirectiveC
};

