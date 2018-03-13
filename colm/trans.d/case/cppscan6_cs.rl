/*
 * @LANG: csharp
 * @GENERATED: true
 */

using System;
// Disables lots of warnings that appear in the test suite
#pragma warning disable 0168, 0169, 0219, 0162, 0414
namespace Test {
class Test
{
int ts;
int te;
int act;
int token;

%%{
	machine scanner;

	action comment {token = 242;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> {token = 193;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> {token = 192;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{token = 195;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> {token = 194;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {token = 218;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {token = 219;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {token = 220;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};

	# Only buffer the second item, first buffered by symbol.
	'::' => {token = 197;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'==' => {token = 223;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'!=' => {token = 224;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'&&' => {token = 225;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'||' => {token = 226;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'*=' => {token = 227;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'/=' => {token = 228;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'%=' => {token = 229;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'+=' => {token = 230;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'-=' => {token = 231;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'&=' => {token = 232;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'^=' => {token = 233;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'|=' => {token = 234;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'++' => {token = 212;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'--' => {token = 213;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'->' => {token = 211;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'->*' => {token = 214;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	'.*' => {token = 215;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};

	# Three char compounds, first item already buffered.
	'...' => {token = 240;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};

	# Single char symbols.
	( punct - [_"'] ) => {token = ( int ) ( data[ts] )
;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => {token = 241;
Console.Write( "<" );Console.Write( token );Console.Write( "> " );Console.Write( new String( data , ts , te - ts ) );
Console.Write( "\n" );};
	*|;
}%%


%% write data;
int cs;

void init()
{
	%% write init;
}

void exec( char[] data, int len )
{
	int p = 0;
	int pe = len;
	int eof = len;
	string _s;
	char [] buffer = new char [1024];
	int blen = 0;
	%% write exec;
}

void finish( )
{
	if ( cs >= scanner_first_final )
		Console.WriteLine( "ACCEPT" );
	else
		Console.WriteLine( "FAIL" );
}

static readonly string[] inp = {
"\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22",
"'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/",
"'\n'\n",
};


static readonly int inplen = 3;

public static void Main (string[] args)
{
	Test machine = new Test();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].ToCharArray(), inp[i].Length );
		machine.finish();
	}
}
}
}
