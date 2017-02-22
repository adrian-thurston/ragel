/*
 * @LANG: java
 * @GENERATED: true
 */


class cppscan6_java
{
int
 ts ;
int
 te ;
int act ;
int token ;

%%{
	machine scanner;

	action comment {token = 242;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> {token = 193;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> {token = 192;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{token = 195;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> {token = 194;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {token = 218;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {token = 219;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {token = 220;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};

	# Only buffer the second item, first buffered by symbol.
	'::' => {token = 197;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'==' => {token = 223;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'!=' => {token = 224;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'&&' => {token = 225;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'||' => {token = 226;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'*=' => {token = 227;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'/=' => {token = 228;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'%=' => {token = 229;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'+=' => {token = 230;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'-=' => {token = 231;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'&=' => {token = 232;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'^=' => {token = 233;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'|=' => {token = 234;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'++' => {token = 212;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'--' => {token = 213;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'->' => {token = 211;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'->*' => {token = 214;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	'.*' => {token = 215;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};

	# Three char compounds, first item already buffered.
	'...' => {token = 240;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};

	# Single char symbols.
	( punct - [_"'] ) => {token = ( int ) ( data[ts] )
;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => {token = 241;
System.out.print( "<" );
System.out.print( token );
System.out.print( "> " );
_s = new String( data, ts, te - ts );
System.out.print( _s );
System.out.print( "\n" );
};
	*|;
}%%



%% write data;
int cs;

void init()
{
	%% write init;
}

void exec( char data[], int len )
{
	char buffer [] = new char[1024];
	int blen = 0;
	int p = 0;
	int pe = len;

	int eof = len;
	String _s;
	%% write exec;
}

void finish( )
{
	if ( cs >= scanner_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22",
"'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/",
"'\n'\n",
};

static final int inplen = 3;

public static void main (String[] args)
{
	cppscan6_java machine = new cppscan6_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
