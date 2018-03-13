(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let ts = ref 0
let te = ref 0
let act = ref 0
let token = ref 0

%%{
	machine scanner;

	action comment {token := 242;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> {token := 193;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> {token := 192;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{token := 195;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> {token := 194;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {token := 218;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {token := 219;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {token := 220;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};

	# Only buffer the second item, first buffered by symbol.
	'::' => {token := 197;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'==' => {token := 223;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'!=' => {token := 224;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'&&' => {token := 225;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'||' => {token := 226;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'*=' => {token := 227;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'/=' => {token := 228;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'%=' => {token := 229;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'+=' => {token := 230;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'-=' => {token := 231;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'&=' => {token := 232;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'^=' => {token := 233;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'|=' => {token := 234;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'++' => {token := 212;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'--' => {token := 213;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'->' => {token := 211;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'->*' => {token := 214;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	'.*' => {token := 215;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};

	# Three char compounds, first item already buffered.
	'...' => {token := 240;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};

	# Single char symbols.
	( punct - [_"'] ) => {token := ( ( Char.code data.[ts.contents] ) )
;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => {token := 241;
print_string( "<" );
print_int( token.contents );
print_string( "> " );
for i = ts.contents to te.contents - 1 do print_char data.[i] done; print_string( "\n" );
};
	*|;
}%%




%% write data;

let exec data = 
  let buffer = String.create(1024) in 
  let blen :int ref = ref 0 in
  let cs = ref 0 in
  let p = ref 0 in
  let pe = ref (String.length data) in
	let eof = pe in
	%% write init;
	%% write exec;
	if !cs >= scanner_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22";
	exec "'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/";
	exec "'\n'\n";
  ()
;;

