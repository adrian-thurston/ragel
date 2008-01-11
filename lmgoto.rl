/*
 * @LANG: c++
 */

#include <iostream>
#include <string.h>
using namespace std;

#define TK_Dlit 192
#define TK_Slit 193
#define TK_Float 194
#define TK_Id 195
#define TK_NameSep 197
#define TK_Arrow 211
#define TK_PlusPlus 212
#define TK_MinusMinus 213
#define TK_ArrowStar 214
#define TK_DotStar 215
#define TK_ShiftLeft 216
#define TK_ShiftRight 217
#define TK_IntegerDecimal 218
#define TK_IntegerOctal 219
#define TK_IntegerHex 220
#define TK_EqualsEquals 223
#define TK_NotEquals 224
#define TK_AndAnd 225
#define TK_OrOr 226
#define TK_MultAssign 227
#define TK_DivAssign 228
#define TK_PercentAssign 229
#define TK_PlusAssign 230
#define TK_MinusAssign 231
#define TK_AmpAssign 232
#define TK_CaretAssign 233
#define TK_BarAssign 234
#define TK_DotDotDot 240
#define TK_Whitespace 241
#define TK_Comment 242

struct Scanner
{
	int cs, act;
	const char *ts, *te;
	bool isCxx;

	void token( int tok );
	void run( const char *buf );
};


%%{
	machine Scanner;

	# Process all comments, relies on isCxx being set.
	comment := |*
		'*/' {
			if ( ! isCxx )
				fgoto main;
			else {
				cout << "comm char: " << ts[0] << endl;
				cout << "comm char: " << ts[1] << endl;
			}
		};

		'\n' {
			if ( isCxx )
				fgoto main;
			else
				cout << "comm char: " << ts[0] << endl;
		};
		
		any {
			cout << "comm char: " << ts[0] << endl;
		};
	*|;
	
	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | /\\./ )* "'" ) { token( TK_Slit );};
	( 'L'? '"' ( [^"\\\n] | /\\./ )* '"' ) { token( TK_Dlit );};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) { token( TK_Id ); };

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) { token( TK_Float );};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]{0,3} ) { token( TK_IntegerDecimal );};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]{0,2} ) { token( TK_IntegerOctal );};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]{0,2} ) ) { token( TK_IntegerHex );};

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
	'...' { token( TK_DotDotDot );};

	# Single char symbols.
	( punct - [_"'] ) { token( ts[0] );};

	# Comments and whitespace. Handle these outside of the machine so that se
	# don't end up buffering the comments.
	'/*' { isCxx = false; fgoto comment; };
	'//' { isCxx = true; fgoto comment; };

	( any - 33..126 )+ { token( TK_Whitespace );};

	*|;
}%%

%% write data nofinal;

void Scanner::token( int tok )
{
	const char *data = ts;
	int len = te - ts;
	cout << "<" << tok << "> ";
	if ( data != 0 ) {
		for ( int i = 0; i < len; i++ )
			cout << data[i];
	}
	cout << '\n';
}

void Scanner::run( const char *buf )
{
	int len = strlen( buf );
	%% write init;
	const char *p = buf;
	const char *pe = buf + len;
	const char *eof = pe;
	%% write exec;

	if ( cs == Scanner_error ) {
		/* Machine failed before finding a token. */
		cout << "PARSE ERROR" << endl;
	}
}

int main()
{
	Scanner scanner;
	scanner.run(
		"//hello*/\n"
		"/*hi there*/ hello 0x88"
	);
	return 0;
}

#ifdef _____OUTPUT_____
comm char: h
comm char: e
comm char: l
comm char: l
comm char: o
comm char: *
comm char: /
comm char: h
comm char: i
comm char:  
comm char: t
comm char: h
comm char: e
comm char: r
comm char: e
<241>  
<195> hello
<241>  
<220> 0x88
#endif
