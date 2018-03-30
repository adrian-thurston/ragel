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
 * JavaScript
 */

const char *js_defaultOutFn( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".js" );
}

HostType hostTypesJS[] =
{
	{ "s8",     0, "int8",    true,   true,  false,  CHAR_MIN,  CHAR_MAX,   0, 0,          1 },
	{ "u8",     0, "uint8",   false,  true,  false,  0, 0,                  0, UCHAR_MAX,  1 },
	{ "s16",    0, "int16",   true,   true,  false,  SHRT_MIN,  SHRT_MAX,   0, 0,          2 },
	{ "u16",    0, "uint16",  false,  true,  false,  0, 0,                  0, USHRT_MAX,  2 },
	{ "i32",    0, "int32",   true,   true,  false,  INT_MIN,   INT_MAX,    0, 0,          4 },
	{ "u32",    0, "uint32",  false,  true,  false,  0, 0,                  0, UINT_MAX,   4 },
	{ "number", 0, "number",  true,   true,  false,  LONG_MIN,  LONG_MAX,   0, 0,          8 },
};

const HostLang hostLangJS = {
	"JavaScript",
	"-P",
	(HostLang::Lang)-1,
	hostTypesJS, 7,
	hostTypesJS+1,
	false,
	true,
	"js",
	&js_defaultOutFn,
	&makeCodeGen,
	Translated,
	VarFeature
};


int main( int argc, const char **argv )
{
	InputData id( &hostLangJS, &rlhc );
	return id.rlhcMain( argc, argv );
}
