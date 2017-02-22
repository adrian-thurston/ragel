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

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
        'a' => {print!( "{}", "pat1\n" );
};

        [ab]+ . 'c' => {print!( "{}", "pat2\n" );
};

        any => {print!( "{}", "any\n" );
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
	unsafe { m( "a".to_string() ); }
}

