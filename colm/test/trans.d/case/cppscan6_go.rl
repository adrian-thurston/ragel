/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var ts  int ;
var te  int ;
var act  int ;
var token  int ;

%%{
	machine scanner;

	action comment {token = 242;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> {token = 193;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> {token = 192;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{token = 195;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> {token = 194;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {token = 218;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {token = 219;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {token = 220;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};

	# Only buffer the second item, first buffered by symbol.
	'::' => {token = 197;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'==' => {token = 223;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'!=' => {token = 224;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'&&' => {token = 225;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'||' => {token = 226;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'*=' => {token = 227;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'/=' => {token = 228;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'%=' => {token = 229;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'+=' => {token = 230;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'-=' => {token = 231;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'&=' => {token = 232;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'^=' => {token = 233;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'|=' => {token = 234;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'++' => {token = 212;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'--' => {token = 213;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'->' => {token = 211;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'->*' => {token = 214;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	'.*' => {token = 215;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};

	# Three char compounds, first item already buffered.
	'...' => {token = 240;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};

	# Single char symbols.
	( punct - [_"'] ) => {token = ( int ) ( data[ts] )
;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => {token = 241;
fmt.Print( "<" );fmt.Print( token );fmt.Print( "> " );fmt.Print( data[ts:te] );fmt.Print( "\n" );};
	*|;
}%%


var cs int;
var blen int;
var buffer [1024] byte;

%% write data;

func prepare() {
	%% write init;
}

func exec(data string) {
	var p int = 0
	var pe int = len(data)
	var eof int = pe
	%% write exec;
}
func finish() {
	if cs >= scanner_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22",
"'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/",
"'\n'\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
