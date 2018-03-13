//
// @LANG: rust
// @GENERATED: true
//

static mut ts : i32
 = 0;
static mut te : i32
 = 0;
static mut act : i32 = 0;
static mut token : i32 = 0;

%%{
	machine scanner;

	action comment {token = 242;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> {token = 193;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> {token = 192;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{token = 195;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> {token = 194;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {token = 218;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {token = 219;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {token = 220;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};

	# Only buffer the second item, first buffered by symbol.
	'::' => {token = 197;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'==' => {token = 223;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'!=' => {token = 224;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'&&' => {token = 225;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'||' => {token = 226;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'*=' => {token = 227;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'/=' => {token = 228;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'%=' => {token = 229;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'+=' => {token = 230;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'-=' => {token = 231;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'&=' => {token = 232;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'^=' => {token = 233;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'|=' => {token = 234;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'++' => {token = 212;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'--' => {token = 213;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'->' => {token = 211;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'->*' => {token = 214;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	'.*' => {token = 215;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};

	# Three char compounds, first item already buffered.
	'...' => {token = 240;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};

	# Single char symbols.
	( punct - [_"'] ) => {token = ( ( data[ts as usize] ) as i32 ) 
;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => {token = 241;
print!( "{}", "<" );
print!( "{}", token );
print!( "{}", "> " );
let s = match std::str::from_utf8(&data[ts as usize .. te as usize]) {
       Ok(v) => v,
       Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
   };
print!( "{}", s );
print!( "{}", "\n" );
};
	*|;
}%%



%% write data;

unsafe fn m( s: String )
{
	let data: &[u8] = s.as_bytes();
	let mut p:i32 = 0;
	let mut pe:i32 = s.len() as i32;
	let mut eof:i32 = s.len() as i32;
	let mut cs: i32 = 0;
	let mut buffer = String::new();

	%% write init;
	%% write exec;

	if ( cs >= scanner_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22".to_string() ); }
	unsafe { m( "'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/".to_string() ); }
	unsafe { m( "'\n'\n".to_string() ); }
}

