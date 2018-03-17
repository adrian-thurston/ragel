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

#include "pcheck.h"
#include "common.h"
#include "stdlib.h"
#include <string.h>
#include <assert.h>
#include "ragel.h"

/*
 * C
 */

const char *c_defaultOutFn( const char *inputFileName )
{
	const char *ext = findFileExtension( inputFileName );
	if ( ext != 0 && strcmp( ext, ".rh" ) == 0 )
		return fileNameFromStem( inputFileName, ".h" );
	else
		return fileNameFromStem( inputFileName, ".c" );
}

HostType hostTypesC[] =
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

const HostLang hostLangC = {
	"C",
	"-C",
	HostLang::C,
	hostTypesC, 8,
	hostTypesC+0,
	true,
	false,
	"c",
	&c_defaultOutFn,
	&makeCodeGen
};

/*
 * ASM
 */
const char *asm_defaultOutFn( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".s" );
}

const HostLang hostLangAsm = {
	"ASM",
	"--asm",
	HostLang::Asm,
	hostTypesC, 8,
	hostTypesC+0,
	true,
	false,
	"no-lang",
	&asm_defaultOutFn,
	&asm_makeCodeGen
};

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
	HostLang::D,
	hostTypesD, 9,
	hostTypesD+2,
	true,
	true,
	"d",
	&d_defaultOutFn,
	&makeCodeGen
};

/*
 * Go
 */

const char *go_defaultOutFn( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".go" );
}

HostType hostTypesGo[] = 
{
	{ "byte",    0,  "uint8",   false,  true,  false,  0, 0,                    U8BIT_MIN,  U8BIT_MAX,   1 },
	{ "int8",    0,  "int8",    true,   true,  false,  S8BIT_MIN,  S8BIT_MAX,   0, 0,                    1 },
	{ "uint8",   0,  "uint8",   false,  true,  false,  0, 0,                    U8BIT_MIN,  U8BIT_MAX,   1 },
	{ "int16",   0,  "int16",   true,   true,  false,  S16BIT_MIN, S16BIT_MAX,  0, 0,                    2 },
	{ "uint16",  0,  "uint16",  false,  true,  false,  0, 0,                    U16BIT_MIN, U16BIT_MAX,  2 },
	{ "int32",   0,  "int32",   true,   true,  false,  S32BIT_MIN, S32BIT_MAX,  0, 0,                    4 },
	{ "uint32",  0,  "uint32",  false,  true,  false,  0, 0,                    U32BIT_MIN, U32BIT_MAX,  4 },
	{ "int64",   0,  "int64",   true,   true,  false,  S64BIT_MIN, S64BIT_MAX,  0, 0,                    8 },
	{ "uint64",  0,  "uint64",  false,  true,  false,  0, 0,                    U64BIT_MIN, U64BIT_MAX,  8 },
	{ "rune",    0,  "int32",   true,   true,  true,   S32BIT_MIN, S32BIT_MAX,  0, 0,                    4 },
};

const HostLang hostLangGo = {
	"Go",
	"-Z",
	HostLang::Go,
	hostTypesGo, 10,
	hostTypesGo+0,
	false,
	true,
	"go",
	&go_defaultOutFn,
	&makeCodeGen
};

/*
 * Java
 */

const char *java_defaultOutFn( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".java" );
}

HostType hostTypesJava[] = 
{
	{ "byte",    0,  "byte",   true,   true,  false,  CHAR_MIN,  CHAR_MAX,    0, 0,           1 },
	{ "short",   0,  "short",  true,   true,  false,  SHRT_MIN,  SHRT_MAX,    0, 0,           2 },
	{ "char",    0,  "char",   false,  true,  false,  0, 0,                   0, USHRT_MAX,   2 },
	{ "int",     0,  "int",    true,   true,  false,  INT_MIN,   INT_MAX,     0, 0,           4 },
};

const HostLang hostLangJava = {
	"Java",
	"-J",
	HostLang::Java,
	hostTypesJava, 4,
	hostTypesJava+2,
	false,
	true,
	"java",
	&java_defaultOutFn,
	&makeCodeGen
};

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
	HostLang::CSharp,
	hostTypesCSharp, 9,
	hostTypesCSharp+4,
	true,
	true,
	"csharp",
	&csharp_defaultOutFn,
	&makeCodeGen
};

/*
 * OCaml
 */
const char *ocaml_defaultOutFn( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".ml" );
}

HostType hostTypesOCaml[] =
{
	{ "int",    0,  "int",      true,   true,  false,  S31BIT_MIN, S31BIT_MAX,  0, 0, 4 },
};

const HostLang hostLangOCaml = {
	"OCaml",
	"-O",
	HostLang::OCaml,
	hostTypesOCaml, 1,
	hostTypesOCaml+0,
	false,
	true,
	"ocaml",
	&ocaml_defaultOutFn,
	&makeCodeGen
};

/*
 * Crack
 */
const char *crack_defaultOutFn( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".crk" );
}

HostType hostTypesCrack[] = 
{
	{ "byte",    0,  "byte",     false,  true,  true,   0, 0,                     0, UCHAR_MAX,    1 },
	{ "int32",   0,  "int32",    true,   true,  false,  S32BIT_MIN,  S32BIT_MAX,  0, 0,            4 },
	{ "uint32",  0,  "uint32",   false,  true,  false,  0, 0,                     0, U32BIT_MAX,   4 },
	{ "int",     0,  "int",      true,   true,  false,  INT_MIN,     INT_MAX,     0, 0,            sizeof(int) },
	{ "uint",    0,  "uint",     false,  true,  false,  0, 0,                     0, UINT_MAX,     sizeof(int) },  
};

const HostLang hostLangCrack = {
	"Crack",
	"-K",
	HostLang::Crack,
	hostTypesCrack, 5,
	hostTypesCrack+0,
	true,
	true,
	"crack",
	&crack_defaultOutFn,
	&makeCodeGen
};

/*
 * Rust
 */
const char *rust_defaultOutFn( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".rs" );
}

HostType hostTypesRust[] =
{
	{ "u8",    0,  "byte",      true,   true,  false,  0, UCHAR_MAX,  0, 0, 4 },
};

const HostLang hostLangRust = {
	"Rust",
	"-U",
	HostLang::Rust,
	hostTypesRust, 1,
	hostTypesRust+0,
	false,
	true,
	"rust",
	&rust_defaultOutFn,
	&makeCodeGen
};

/*
 * Julia
 */
const char *julia_defaultOutFn( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".jl" );
}

HostType hostTypesJulia[] =
{
	{ "u8",    0,  "byte",      true,   true,  false,  0, UCHAR_MAX,  0, 0, 4 },
};

const HostLang hostLangJulia = {
	"Julia",
	"-Y",
	HostLang::Julia,
	hostTypesJulia, 1,
	hostTypesJulia+0,
	false,
	true,
	"julia",
	&julia_defaultOutFn,
	&makeCodeGen
};

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
	HostLang::JS,
	hostTypesJS, 7,
	hostTypesJS+1,
	false,
	true,
	"js",
	&js_defaultOutFn,
	&makeCodeGen
};

const HostLang *hostLangs[] = {
	&hostLangC,
	&hostLangAsm,
	&hostLangD,
	&hostLangGo,
	&hostLangJava,
	&hostLangCSharp,
	&hostLangOCaml,
	&hostLangRust,
	&hostLangCrack,
	&hostLangJulia,
	&hostLangJS,
};

const int numHostLangs = sizeof(hostLangs)/sizeof(hostLangs[0]);

HostType *findAlphType( const HostLang *hostLang, const char *s1 )
{
	for ( int i = 0; i < hostLang->numHostTypes; i++ ) {
		if ( strcmp( s1, hostLang->hostTypes[i].data1 ) == 0 && 
				hostLang->hostTypes[i].data2 == 0 )
		{
			return hostLang->hostTypes + i;
		}
	}

	return 0;
}

HostType *findAlphType( const HostLang *hostLang, const char *s1, const char *s2 )
{
	for ( int i = 0; i < hostLang->numHostTypes; i++ ) {
		if ( strcmp( s1, hostLang->hostTypes[i].data1 ) == 0 && 
				hostLang->hostTypes[i].data2 != 0 && 
				strcmp( s2, hostLang->hostTypes[i].data2 ) == 0 )
		{
			return hostLang->hostTypes + i;
		}
	}

	return 0;
}

HostType *findAlphTypeInternal( const HostLang *hostLang, const char *s1 )
{
	for ( int i = 0; i < hostLang->numHostTypes; i++ ) {
		if ( strcmp( s1, hostLang->hostTypes[i].internalName ) == 0 )
			return hostLang->hostTypes + i;
	}

	return 0;
}

/* Construct a new parameter checker with for paramSpec. */
ParamCheck::ParamCheck( const char *paramSpec, int argc, const char **argv )
:
	state(noparam),
	argOffset(0),
	curArg(0),
	iCurArg(1),
	paramSpec(paramSpec), 
	argc(argc), 
	argv(argv)
{
}

/* Check a single option. Returns the index of the next parameter.  Sets p to
 * the arg character if valid, 0 otherwise.  Sets parg to the parameter arg if
 * there is one, NULL otherwise. */
bool ParamCheck::check()
{
	bool requiresParam;

	if ( iCurArg >= argc ) {            /* Off the end of the arg list. */
		state = noparam;
		return false;
	}

	if ( argOffset != 0 && *argOffset == 0 ) {
		/* We are at the end of an arg string. */
		iCurArg += 1;
		if ( iCurArg >= argc ) {
			state = noparam;
			return false;
		}
		argOffset = 0;
	}

	if ( argOffset == 0 ) {
		/* Set the current arg. */
		curArg = argv[iCurArg];

		/* We are at the beginning of an arg string. */
		if ( argv[iCurArg] == 0 ||        /* Argv[iCurArg] is null. */
			 argv[iCurArg][0] != '-' ||   /* Not a param. */
			 argv[iCurArg][1] == 0 ) {    /* Only a dash. */
			parameter = 0;
			paramArg = 0;

			iCurArg += 1;
			state = noparam;
			return true;
		}
		argOffset = argv[iCurArg] + 1;
	}

	/* Get the arg char. */
	char argChar = *argOffset;
	
	/* Loop over all the parms and look for a match. */
	const char *pSpec = paramSpec;
	while ( *pSpec != 0 ) {
		char pSpecChar = *pSpec;

		/* If there is a ':' following the char then
		 * it requires a parm.  If a parm is required
		 * then move ahead two in the parmspec. Otherwise
		 * move ahead one in the parm spec. */
		if ( pSpec[1] == ':' ) {
			requiresParam = true;
			pSpec += 2;
		}
		else {
			requiresParam = false;
			pSpec += 1;
		}

		/* Do we have a match. */
		if ( argChar == pSpecChar ) {
			if ( requiresParam ) {
				if ( argOffset[1] == 0 ) {
					/* The param must follow. */
					if ( iCurArg + 1 == argc ) {
						/* We are the last arg so there
						 * cannot be a parameter to it. */
						parameter = argChar;
						paramArg = 0;
						iCurArg += 1;
						argOffset = 0;
						state = invalid;
						return true;
					}
					else {
						/* the parameter to the arg is the next arg. */
						parameter = pSpecChar;
						paramArg = argv[iCurArg + 1];
						iCurArg += 2;
						argOffset = 0;
						state = match;
						return true;
					}
				}
				else {
					/* The param for the arg is built in. */
					parameter = pSpecChar;
					paramArg = argOffset + 1;
					iCurArg += 1;
					argOffset = 0;
					state = match;
					return true;
				}
			}
			else {
				/* Good, we matched the parm and no
				 * arg is required. */
				parameter = pSpecChar;
				paramArg = 0;
				argOffset += 1;
				state = match;
				return true;
			}
		}
	}

	/* We did not find a match. Bad Argument. */
	parameter = argChar;
	paramArg = 0;
	argOffset += 1;
	state = invalid;
	return true;
}

/* Counts newlines before sending sync. */
int output_filter::sync( )
{
	line += 1;
	return std::filebuf::sync();
}

std::streamsize output_filter::countAndWrite( const char *s, std::streamsize n )
{
	for ( int i = 0; i < n; i++ ) {
		switch ( s[i] ) {
		case '\n':
			line += 1;
			break;
		case '{':
			level += 1;
			break;
		case '}':
			level -= 1;
			break;
		}
	}

	return std::filebuf::xsputn( s, n );
}

/* Counts newlines before sending data out to file. */
std::streamsize output_filter::xsputn( const char *s, std::streamsize n )
{
	std::streamsize ret = n;
	int l;

restart:
	if ( indent ) {
		/* Consume mode Looking for the first non-whitespace. */
		while ( n > 0 && ( *s == ' ' || *s == '\t' ) ) {
			s += 1;
			n -= 1;
		}

		if ( n > 0 ) {
			int tabs = level;

			if ( *s == '}' ) {
				/* If the next char is de-dent, then reduce the tabs. This is
				 * not a stream state change. The level reduction will be
				 * computed in write. */
				tabs -= 1;
			}

			/* Found some data, print the indentation and turn off indentation
			 * mode. */
			for ( l = 0; l < tabs; l++ )
				countAndWrite( "\t", 1 );

			indent = 0;

			goto restart;
		}
	}
	else {
		char *nl;
		if ( (nl = (char*)memchr( s, '\n', n )) ) {
			/* Print up to and including the newline. */
			int wl = nl - s + 1;
			countAndWrite( s, wl );

			/* Go into consume state. If we see more non-indentation chars we
			 * will generate the appropriate indentation level. */
			s += wl;
			n -= wl;
			indent = true;
			goto restart;
		}
		else {
			/* Indentation off, or no indent trigger (newline). */
			countAndWrite( s, n );
		}
	}

	// What to do here?
	return ret;
}

/* Scans a string looking for the file extension. If there is a file
 * extension then pointer returned points to inside the string
 * passed in. Otherwise returns null. */
const char *findFileExtension( const char *stemFile )
{
	const char *ppos = stemFile + strlen(stemFile) - 1;

	/* Scan backwards from the end looking for the first dot.
	 * If we encounter a '/' before the first dot, then stop the scan. */
	while ( 1 ) {
		/* If we found a dot or got to the beginning of the string then
		 * we are done. */
		if ( ppos == stemFile || *ppos == '.' )
			break;

		/* If we hit a / then there is no extension. Done. */
		if ( *ppos == '/' ) {
			ppos = stemFile;
			break;
		}
		ppos--;
	} 

	/* If we got to the front of the string then bail we 
	 * did not find an extension  */
	if ( ppos == stemFile )
		ppos = 0;

	return ppos;
}

/* Make a file name from a stem. Removes the old filename suffix and
 * replaces it with a new one. Returns a newed up string. */
const char *fileNameFromStem( const char *stemFile, const char *suffix )
{
	long len = strlen( stemFile );
	assert( len > 0 );

	/* Get the extension. */
	const char *ppos = findFileExtension( stemFile );

	/* If an extension was found, then shorten what we think the len is. */
	if ( ppos != 0 )
		len = ppos - stemFile;

	/* Make the return string from the stem and the suffix. */
	char *retVal = new char[ len + strlen( suffix ) + 1 ];
	strncpy( retVal, stemFile, len );
	strcpy( retVal + len, suffix );

	return retVal;
}

exit_object endp;

void operator<<( std::ostream &out, exit_object & )
{
	out << std::endl;
	throw AbortCompile( 1 );
}
