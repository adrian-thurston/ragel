#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine scanner;

	action comment {token = 242;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> {token = 193;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> {token = 192;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{token = 195;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> {token = 194;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {token = 218;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {token = 219;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {token = 220;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};

	# Only buffer the second item, first buffered by symbol.
	'::' => {token = 197;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'==' => {token = 223;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'!=' => {token = 224;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'&&' => {token = 225;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'||' => {token = 226;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'*=' => {token = 227;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'/=' => {token = 228;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'%=' => {token = 229;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'+=' => {token = 230;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'-=' => {token = 231;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'&=' => {token = 232;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'^=' => {token = 233;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'|=' => {token = 234;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'++' => {token = 212;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'--' => {token = 213;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'->' => {token = 211;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'->*' => {token = 214;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	'.*' => {token = 215;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};

	# Three char compounds, first item already buffered.
	'...' => {token = 240;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};

	# Single char symbols.
	( punct - [_"'] ) => {token = ( data[ts].ord )
;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => {token = 241;
print( "<" );
print( token );
print( "> " );
_m = data[ts..te-1];
print( _m );
print( "\n" );
};
	*|;
}%%



%% write data;

def run_machine( data )
	p = 0
	pe = data.length
	eof = data.length
	cs = 0;
	_m = 
	_a = 
	buffer = Array.new
	blen = 0
ts = 1
te = 1
act = 1
token = 1
	%% write init;
	%% write exec;
	if cs >= scanner_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22",
"'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/",
"'\n'\n",
]

inplen = 3

inp.each { |str| run_machine(str) }

