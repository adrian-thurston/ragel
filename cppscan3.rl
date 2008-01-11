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

#define BUFSIZE 4096

char buf[BUFSIZE];

struct Scanner
{
	int cs, act;
	const char *ts, *te;

	void token( int tok );
	void run();

	void init( );
	void execute( const char *data, int len );
	int finish( );
};

%%{
	machine Scanner;

	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | /\\./ )* "'" ) 
		=> { token( TK_Slit );};
	( 'L'? '"' ( [^"\\\n] | /\\./ )* '"' ) 
		=> { token( TK_Dlit );};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{ token( TK_Id );};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> { token( TK_Float );};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]{0,3} ) 
		=> { token( TK_IntegerDecimal );};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]{0,2} ) 
		=> { token( TK_IntegerOctal );};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]{0,2} ) ) 
		=> { token( TK_IntegerHex );};

	# Only buffer the second item, first buffered by symbol. */
	'::' => {token( TK_NameSep );};
	'==' => {token( TK_EqualsEquals );};
	'!=' => {token( TK_NotEquals );};
	'&&' => {token( TK_AndAnd );};
	'||' => {token( TK_OrOr );};
	'*=' => {token( TK_MultAssign );};
	'/=' => {token( TK_DivAssign );};
	'%=' => {token( TK_PercentAssign );};
	'+=' => {token( TK_PlusAssign );};
	'-=' => {token( TK_MinusAssign );};
	'&=' => {token( TK_AmpAssign );};
	'^=' => {token( TK_CaretAssign );};
	'|=' => {token( TK_BarAssign );};
	'++' => {token( TK_PlusPlus );};
	'--' => {token( TK_MinusMinus );};
	'->' => {token( TK_Arrow );};
	'->*' => {token( TK_ArrowStar );};
	'.*' => {token( TK_DotStar );};

	# Three char compounds, first item already buffered. */
	'...' => { token( TK_DotDotDot );};

	# Single char symbols.
	( punct - [_"'] ) => { token( ts[0] );};

	action comment {
		token( TK_Comment );
	}

	# Comments and whitespace.
	'/*' ( any* $0 '*/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => { token( TK_Whitespace );};

	*|;
}%%

%% write data;

void Scanner::init( )
{
	%% write init;
}

/* Returns the count of bytes still in the buffer 
 * (shifted to the biginning) */
void Scanner::execute( const char *data, int len )
{
	const char *p = data;
	const char *pe = data + len;
	const char *eof = pe;

	%% write exec;

	cout << "P: " << (p - data) << endl;
}

int Scanner::finish( )
{
	if ( cs == Scanner_error )
		return -1;
	if ( cs >= Scanner_first_final )
		return 1;
	return 0;
}


void Scanner::token( int tok )
{
	const char *data = ts;
	int len = te - ts;
	cout << "<" << tok << "> ";
	for ( int i = 0; i < len; i++ )
		cout << data[i];
	cout << '\n';
}

void test( const char *buf )
{
	int len = strlen( buf );
	std::ios::sync_with_stdio(false);
	Scanner scanner;
	scanner.init();

	scanner.execute( buf, len );
	if ( scanner.cs == Scanner_error ) {
		/* Machine failed before finding a token. */
		cout << "PARSE ERROR" << endl;
	}

	/* FIXME: Last token may get lost. */
	scanner.finish();
}

int main()
{
	test(
		"\"\\\"hi\" /*\n"
		"*/\n"
		"44 .44\n"
		"44. 44\n"
		"44 . 44\n"
		"44.44\n"
		"_hithere22"
	);

	test(
		"'\\''\"\\n\\d'\\\"\"\n"
		"hi\n"
		"99\n"
		".99\n"
		"99e-4\n"
		"->*\n"
		"||\n"
		"0x98\n"
		"0x\n"
		"//\n"
		"/* * */"
	);

	test(
		"'\n"
		"'\n"
	);

}

#ifdef _____OUTPUT_____
<192> "\"hi"
<241>  
<242> /*
*/
<241> 

<218> 44
<241>  
<194> .44
<241> 

<194> 44.
<241>  
<218> 44
<241> 

<218> 44
<241>  
<46> .
<241>  
<218> 44
<241> 

<194> 44.44
<241> 

<195> _hithere22
P: 51
<193> '\''
<192> "\n\d'\""
<241> 

<195> hi
<241> 

<218> 99
<241> 

<194> .99
<241> 

<194> 99e-4
<241> 

<214> ->*
<241> 

<226> ||
<241> 

<220> 0x98
<241> 

<218> 0
<195> x
<241> 

<242> //

<242> /* * */
P: 55
P: 1
PARSE ERROR
#endif
