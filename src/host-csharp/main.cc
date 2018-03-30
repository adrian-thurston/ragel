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

extern struct colm_sections rlhc;

/*
 * C#
 */

const char *csharp_defaultOutFn( const char *inputFileName )
{
	const char *ext = findFileExtension( inputFileName );
	if ( ext != 0 && strcmp( ext, ".rh" ) == 0 )
		return fileNameFromStem( inputFileName, ".h" );
	else
		return fileNameFromStem( inputFileName, ".cs" );
}

HostType hostTypesCSharp[] =
{
	{ "sbyte",   0,  "sbyte",   true,   true,  false,  CHAR_MIN,  CHAR_MAX,    0, 0,           1 },
	{ "byte",    0,  "byte",    false,  true,  false,  0, 0,                   0, UCHAR_MAX,   1 },
	{ "short",   0,  "short",   true,   true,  false,  SHRT_MIN,  SHRT_MAX,    0, 0,           2 },
	{ "ushort",  0,  "ushort",  false,  true,  false,  0, 0,                   0, USHRT_MAX,   2 },
	{ "char",    0,  "char",    false,  true,  true,   0, 0,                   0, USHRT_MAX,   2 },
	{ "int",     0,  "int",     true,   true,  false,  INT_MIN,   INT_MAX,     0, 0,           4 },
	{ "uint",    0,  "uint",    false,  true,  false,  0, 0,                   0, UINT_MAX,    4 },
	{ "long",    0,  "long",    true,   true,  false,  LONG_MIN,  LONG_MAX,    0, 0,           8 },
	{ "ulong",   0,  "ulong",   false,  true,  false,  0, 0,                   0, ULONG_MAX,   8 },
};

const HostLang hostLangCSharp = {
	"C#",
	"-A",
	(HostLang::Lang)-1,
	hostTypesCSharp, 9,
	hostTypesCSharp+4,
	true,
	true,
	"csharp",
	&csharp_defaultOutFn,
	&makeCodeGen,
	Translated,
	GotoFeature
};



int main( int argc, const char **argv )
{
	InputData id( &hostLangCSharp, &rlhc );
	return id.rlhcMain( argc, argv );
}
