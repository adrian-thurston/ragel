/*
 * @LANG: d
 */

/*
 * Test in and out state actions.
 */

import std.c.stdio;
import std.string;

static const int TK_Dlit = 192;
static const int TK_Slit = 193;
static const int TK_Float = 194;
static const int TK_Id = 195;
static const int TK_NameSep = 197;
static const int TK_Arrow = 211;
static const int TK_PlusPlus = 212;
static const int TK_MinusMinus = 213;
static const int TK_ArrowStar = 214;
static const int TK_DotStar = 215;
static const int TK_ShiftLeft = 216;
static const int TK_ShiftRight = 217;
static const int TK_IntegerDecimal = 218;
static const int TK_IntegerOctal = 219;
static const int TK_IntegerHex = 220;
static const int TK_EqualsEquals = 223;
static const int TK_NotEquals = 224;
static const int TK_AndAnd = 225;
static const int TK_OrOr = 226;
static const int TK_MultAssign = 227;
static const int TK_DivAssign = 228;
static const int TK_PercentAssign = 229;
static const int TK_PlusAssign = 230;
static const int TK_MinusAssign = 231;
static const int TK_AmpAssign = 232;
static const int TK_CaretAssign = 233;
static const int TK_BarAssign = 234;
static const int TK_DotDotDot = 240;
static const int TK_Whitespace = 241;
static const int TK_Comment = 242;

class Scanner 
{
	int cs, act;
	char *ts, te;

	void token( int tok )
	{
		char *data = ts;
		int len = te - ts;
		printf( "<%i> ", tok );
		for ( int i = 0; i < len; i++ )
			printf( "%c", data[i] );
		printf( "\n" );
	}

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

	%% write data noprefix;

	void init( )
	{
		%% write init;
	}

	void execute( char* data, int len )
	{
		char *p = data;
		char *pe = data + len;
		char *eof = pe;

		%% write exec;
	}

	// Indicate that there is no more data. Returns -1 if the machine finishes
	// in the error state and does not accept, 0 if the machine finishes
	// in any other non-accepting state and 1 if the machine finishes in an
	// accepting state.
	int finish( )
	{
		if ( cs == error )
			return -1;
		if ( cs >= first_final )
			return 1;
		return 0;
	}
};

static const int BUFSIZE = 12;

void test( char buf[] )
{
	Scanner scanner = new Scanner();
	scanner.init();

	scanner.execute( buf.ptr, buf.length );
	if ( scanner.cs == Scanner.error ) {
		/* Machine failed before finding a token. */
		printf("PARSE ERROR\n");
	}
	scanner.finish();
	return 0;
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

	return 0;
}

/+ _____OUTPUT_____
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
PARSE ERROR
+++++++++++++++++++/
