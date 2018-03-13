/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class scanner
{
const(char) * ts;
const(char) * te;
int act;
int token;

%%{
	machine scanner;

	action comment {token = 242;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> {token = 193;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> {token = 192;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{token = 195;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> {token = 194;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {token = 218;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {token = 219;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {token = 220;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};

	# Only buffer the second item, first buffered by symbol.
	'::' => {token = 197;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'==' => {token = 223;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'!=' => {token = 224;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'&&' => {token = 225;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'||' => {token = 226;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'*=' => {token = 227;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'/=' => {token = 228;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'%=' => {token = 229;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'+=' => {token = 230;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'-=' => {token = 231;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'&=' => {token = 232;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'^=' => {token = 233;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'|=' => {token = 234;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'++' => {token = 212;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'--' => {token = 213;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'->' => {token = 211;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'->*' => {token = 214;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	'.*' => {token = 215;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};

	# Three char compounds, first item already buffered.
	'...' => {token = 240;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};

	# Single char symbols.
	( punct - [_"'] ) => {token = cast( int ) ( ts[0] )
;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => {token = 241;
printf( "%.*s", "<" );printf( "%d", token );
printf( "%.*s", "> " );printf( "%.*s", ts[0..(te - ts)] );printf( "%.*s", "\n" );};
	*|;
}%%



%% write data;
int cs;
int blen;
char buffer[1024];

void init()
{
	%% write init;
}
void exec( const(char) data[] )
{
	const(char) *p = data.ptr;
	const(char) *pe = data.ptr + data.length;
	const(char) *eof = pe;
	char _s[];

	%% write exec;
}

void finish( )
{
	if ( cs >= scanner_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22",
"'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/",
"'\n'\n",
];

int inplen = 3;

}
int main()
{
	scanner m = new scanner();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

