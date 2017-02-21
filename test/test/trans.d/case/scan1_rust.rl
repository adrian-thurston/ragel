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
		'a' => {print!( "{}", "on last     " );
if ( p + 1 == te )
{
	print!( "{}", "yes" );

} 
print!( "{}", "\n" );
};

		'b'+ => {print!( "{}", "on next     " );
if ( p + 1 == te )
{
	print!( "{}", "yes" );

} 
print!( "{}", "\n" );
};

		'c1' 'dxxx'? => {print!( "{}", "on lag      " );
if ( p + 1 == te )
{
	print!( "{}", "yes" );

} 
print!( "{}", "\n" );
};

		'd1' => {print!( "{}", "lm switch1  " );
if ( p + 1 == te )
{
	print!( "{}", "yes" );

} 
print!( "{}", "\n" );
};
		'd2' => {print!( "{}", "lm switch2  " );
if ( p + 1 == te )
{
	print!( "{}", "yes" );

} 
print!( "{}", "\n" );
};

		[d0-9]+ '.';

		'\n';
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
	unsafe { m( "abbc1d1d2\n".to_string() ); }
}

