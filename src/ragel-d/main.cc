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
 * D
 */

const char *d_defaultOutFn( const char *inputFileName )
{
	const char *ext = findFileExtension( inputFileName );
	if ( ext != 0 && strcmp( ext, ".rh" ) == 0 )
		return fileNameFromStem( inputFileName, ".h" );
	else
		return fileNameFromStem( inputFileName, ".d" );
}

HostType hostTypesD[] =
{
	{ "byte",    0,  "byte",    true,   true,  false,  CHAR_MIN,  CHAR_MAX,    0, 0,           1 },
	{ "ubyte",   0,  "ubyte",   false,  true,  false,  0, 0,                   0, UCHAR_MAX,   1 },
	{ "char",    0,  "char",    false,  true,  false,  0, 0,                   0, UCHAR_MAX,   1 },
	{ "short",   0,  "short",   true,   true,  false,  SHRT_MIN,  SHRT_MAX,    0, 0,           2 },
	{ "ushort",  0,  "ushort",  false,  true,  false,  0, 0,                   0, USHRT_MAX,   2 },
	{ "wchar",   0,  "wchar",   false,  true,  false,  0, 0,                   0, USHRT_MAX,   2 },
	{ "int",     0,  "int",     true,   true,  false,  INT_MIN,   INT_MAX,     0, 0,           4 },
	{ "uint",    0,  "uint",    false,  true,  false,  0, 0,                   0, UINT_MAX,    4 },
	{ "dchar",   0,  "dchar",   false,  true,  false,  0, 0,                   0, UINT_MAX,    4 },
};

const HostLang hostLangD = {
	"D",
	"-D",
	(HostLang::Lang)-1,
	hostTypesD, 9,
	hostTypesD+2,
	true,
	true,
	"d",
	&d_defaultOutFn,
	&makeCodeGen,
	Translated,
	GotoFeature
};


int main( int argc, const char **argv )
{
	InputData id( &hostLangD, &rlhc );
	return id.rlhcMain( argc, argv );
}
