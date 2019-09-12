/*
 * A C++ scanner. Uses the longest match construction.
 * << <= <<= >> >= >>= are left out since angle brackets are used in templates.
 */

#include <string.h>
#include <stdlib.h>
#include <iostream>

#define TK_Dlit 256
#define TK_Slit 257
#define TK_Float 258
#define TK_Id 259
#define TK_NameSep 260
#define TK_Arrow 261
#define TK_PlusPlus 262
#define TK_MinusMinus 263
#define TK_ArrowStar 264
#define TK_DotStar 265
#define TK_ShiftLeft 266
#define TK_ShiftRight 267
#define TK_IntegerDecimal 268
#define TK_IntegerOctal 269
#define TK_IntegerHex 270
#define TK_EqualsEquals 271
#define TK_NotEquals 272
#define TK_AndAnd 273
#define TK_OrOr 274
#define TK_MultAssign 275
#define TK_DivAssign 276
#define TK_PercentAssign 277
#define TK_PlusAssign 278
#define TK_MinusAssign 279
#define TK_AmpAssign 280
#define TK_CaretAssign 281
#define TK_BarAssign 282
#define TK_DotDotDot 283
#define TK_Whitespace 284
#define TK_Comment 285

#define BUFSIZE 16384

/* EOF char used to flush out that last token. This should be a whitespace
 * token. */

#define LAST_CHAR 0

using std::cerr;
using std::cout;
using std::cin;
using std::endl;

static char buf[BUFSIZE];
static int line = 1, col = 1;
static char *ts, *te;
static int act, have = 0;
static int cs;

%%{
	machine Scanner; 
	write data nofinal;

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	c_comment := 
		any* :>> '*/'
		@{ fgoto main; };

	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | /\\./ )* "'" ) 
		{token( TK_Slit );};
	( 'L'? '"' ( [^"\\\n] | /\\./ )* '"' ) 
		{token( TK_Dlit );};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		{token( TK_Id );};

	# Floating literals.
	( fract_const exponent? float_suffix? | digit+ exponent float_suffix? ) 
		{token( TK_Float );};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]{0,3} ) 
		{token( TK_IntegerDecimal );};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]{0,2} ) 
		{token( TK_IntegerOctal );};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]{0,2} ) ) 
		{token( TK_IntegerHex );};

	# Only buffer the second item, first buffered by symbol. */
	'::' {token( TK_NameSep );};
	'==' {token( TK_EqualsEquals );};
	'!=' {token( TK_NotEquals );};
	'&&' {token( TK_AndAnd );};
	'||' {token( TK_OrOr );};
	'*=' {token( TK_MultAssign );};
	'/=' {token( TK_DivAssign );};
	'%=' {token( TK_PercentAssign );};
	'+=' {token( TK_PlusAssign );};
	'-=' {token( TK_MinusAssign );};
	'&=' {token( TK_AmpAssign );};
	'^=' {token( TK_CaretAssign );};
	'|=' {token( TK_BarAssign );};
	'++' {token( TK_PlusPlus );};
	'--' {token( TK_MinusMinus );};
	'->' {token( TK_Arrow );};
	'->*' {token( TK_ArrowStar );};
	'.*' {token( TK_DotStar );};

	# Three char compounds, first item already buffered. */
	'...' {token( TK_DotDotDot );};

	# Single char symbols.
	( punct - [_"'] ) {token( ts[0] );};

	# Comments and whitespace.
	'/*' { fgoto c_comment; };
	'//' [^\n]* '\n';
	( any - 33..126 )+;

	*|;
}%%

void token( int tok )
{
	char *data = ts;
	int len = te - ts;

	cout << '<' << tok << "> ";
	cout.write( data, len );
	cout << '\n';
	
	/* Count newlines and columns. This code is here mainly for having some
	 * code in the token routine when commenting out the above output during
	 * performance testing. */
	for ( int i = 0; i < len; i ++ ) {
		if ( data[i] == '\n' ) {
			line += 1;
			col = 1;
		}
		else {
			col += 1;
		}
	}
}

int main()
{
	std::ios::sync_with_stdio(false);

	%% write init;

	/* Do the first read. */
	bool done = false;
	while ( !done ) {
		char *p = buf + have;
		int space = BUFSIZE - have;

		if ( space == 0 ) {
			/* We filled up the buffer trying to scan a token. */
			cerr << "OUT OF BUFFER SPACE" << endl;
			exit(1);
		}

		cin.read( p, space );
		int len = cin.gcount();
		char *pe = p + len;
		char *eof = 0;

		/* If we see eof then append the EOF char. */
	 	if ( cin.eof() ) {
			eof = pe;
			done = true;
		}

		%% write exec;

		/* Check if we failed. */
		if ( cs == Scanner_error ) {
			/* Machine failed before finding a token. */
			cerr << "PARSE ERROR" << endl;
			exit(1);
		}

		/* Now set up the prefix. */
		if ( ts == 0 )
			have = 0;
		else {
			/* There is data that needs to be shifted over. */
			have = pe - ts;
			memmove( buf, ts, have );
			te -= (ts-buf);
			ts = buf;
		}
	}

	return 0;
}
