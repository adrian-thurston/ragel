//
// @LANG: rust
// @GENERATED: true
//



%%{
	machine rangei;

	main := 
		'a' ../i 'z' .
		'A' ../i 'Z' .
		60 ../i 93 .
		94 ../i 125 .
		86 ../i 101 .
		60 ../i 125
		'';
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

	if ( cs >= rangei_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "AaBbAa".to_string() ); }
	unsafe { m( "Aa`bAa".to_string() ); }
	unsafe { m( "AaB@Aa".to_string() ); }
	unsafe { m( "AaBbMa".to_string() ); }
	unsafe { m( "AaBbma".to_string() ); }
}

