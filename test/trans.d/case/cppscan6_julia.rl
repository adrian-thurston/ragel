//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine scanner;

	action comment {token = 242;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> {token = 193;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> {token = 192;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{token = 195;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
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
print( data[(ts+1) : (te)] )
print( "\n" );
};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {token = 218;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {token = 219;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {token = 220;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};

	# Only buffer the second item, first buffered by symbol.
	'::' => {token = 197;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'==' => {token = 223;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'!=' => {token = 224;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'&&' => {token = 225;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'||' => {token = 226;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'*=' => {token = 227;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'/=' => {token = 228;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'%=' => {token = 229;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'+=' => {token = 230;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'-=' => {token = 231;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'&=' => {token = 232;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'^=' => {token = 233;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'|=' => {token = 234;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'++' => {token = 212;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'--' => {token = 213;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'->' => {token = 211;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'->*' => {token = 214;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	'.*' => {token = 215;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};

	# Three char compounds, first item already buffered.
	'...' => {token = 240;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};

	# Single char symbols.
	( punct - [_"'] ) => {token = convert( Int, ( data[ts+1] ) ) 
;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => {token = 241;
print( "<" );
print( token );
print( "> " );
print( data[(ts+1) : (te)] )
print( "\n" );
};
	*|;
}%%



%% write data;

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""
ts = 0;
te = 0;
act = 0;
token = 0;

	%% write init;
	%% write exec;

	if ( cs >= scanner_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22" );
	m( "'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/" );
	m( "'\n'\n" );
