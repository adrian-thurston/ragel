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

extern struct colm_sections rlparseOCaml;
extern struct colm_sections rlhcOCaml;

/*
 * OCaml
 */
const char *defaultOutFnOCaml( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".ml" );
}

HostType hostTypesOCaml[] =
{
	{ "int",    0,  "int",      true,   true,  false,  S31BIT_MIN, S31BIT_MAX,  0, 0, 4 },
};

const HostLang hostLangOCaml =
{
	hostTypesOCaml,
	1,
	0,
	false,
	false, /* loopLabels */
	Translated,
	VarFeature,
	&makeCodeGen,
	&defaultOutFnOCaml,
	&genLineDirectiveTrans
};

int main( int argc, const char **argv )
{
	InputData id( &hostLangOCaml, &rlparseOCaml, &rlhcOCaml );
	return id.rlhcMain( argc, argv );
}
