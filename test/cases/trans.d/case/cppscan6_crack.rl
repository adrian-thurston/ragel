//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

int
 ts;
int
 te;
int act;
int token;

%%{
	machine scanner;

	action comment {token = 242;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> {token = 193;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> {token = 192;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{token = 195;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> {token = 194;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {token = 218;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {token = 219;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {token = 220;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};

	# Only buffer the second item, first buffered by symbol.
	'::' => {token = 197;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'==' => {token = 223;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'!=' => {token = 224;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'&&' => {token = 225;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'||' => {token = 226;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'*=' => {token = 227;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'/=' => {token = 228;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'%=' => {token = 229;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'+=' => {token = 230;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'-=' => {token = 231;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'&=' => {token = 232;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'^=' => {token = 233;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'|=' => {token = 234;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'++' => {token = 212;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'--' => {token = 213;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'->' => {token = 211;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'->*' => {token = 214;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	'.*' => {token = 215;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};

	# Three char compounds, first item already buffered.
	'...' => {token = 240;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};

	# Single char symbols.
	( punct - [_"'] ) => {token = ( int ( data[ts] ) ) 
;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => {token = 241;
cout.format( "<" );
cout.format( token );
cout.format( "> " );
int len = uintz(te) - uintz(ts);
cout.write( Buffer(data + uintz(ts), len) );
cout.format( "\n" );
};
	*|;
}%%



%% write data;

void m( String s )
{
	byteptr data = s.buffer;
	int p = 0;
	int pe = s.size;
	int cs;
	String buffer;
	int eof = pe;

	%% write init;
	%% write exec;

	if ( cs >= scanner_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22" );
	m( "'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/" );
	m( "'\n'\n" );
}

main();
