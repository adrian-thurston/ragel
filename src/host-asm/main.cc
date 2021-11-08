/*
 * Copyright 2001-2018 Adrian Thurston <thurston@colm.net>
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

#include "inputdata.h"
#include "nragel.h"
#include <libfsm/asm.h>

extern struct colm_sections rlparseAsm;

extern "C" const HostLang hostLangAsm;

/*
 * ASM
 */
const char *defaultOutFnAsm( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".s" );
}

HostType hostTypesAsm[] =
{
	{ "char",     0,       "char",    true,   true,  false,  CHAR_MIN,  CHAR_MAX,   0, 0,          sizeof(char) },
	{ "unsigned", "char",  "uchar",   false,  true,  false,  0, 0,                  0, UCHAR_MAX,  sizeof(unsigned char) },
	{ "short",    0,       "short",   true,   true,  false,  SHRT_MIN,  SHRT_MAX,   0, 0,          sizeof(short) },
	{ "unsigned", "short", "ushort",  false,  true,  false,  0, 0,                  0, USHRT_MAX,  sizeof(unsigned short) },
	{ "int",      0,       "int",     true,   true,  false,  INT_MIN,   INT_MAX,    0, 0,          sizeof(int) },
	{ "unsigned", "int",   "uint",    false,  true,  false,  0, 0,                  0, UINT_MAX,   sizeof(unsigned int) },
	{ "long",     0,       "long",    true,   true,  false,  LONG_MIN,  LONG_MAX,   0, 0,          sizeof(long) },
	{ "unsigned", "long",  "ulong",   false,  true,  false,  0, 0,                  0, ULONG_MAX,  sizeof(unsigned long) },
};

extern "C" const HostLang hostLangAsm = {
	hostTypesAsm,
	8,
	0,
	true,
	false, /* loopLabels */
	Direct,
	GotoFeature,
	&makeCodeGenAsm,
	&defaultOutFnAsm,
	&genLineDirectiveAsm
};

int main( int argc, const char **argv )
{
	InputData id( &hostLangAsm, &rlparseAsm, 0 );
	return id.main( argc, argv );
}

