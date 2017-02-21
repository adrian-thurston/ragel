//
// @LANG: rust
// @GENERATED: true
//



%%{
	machine indep;

	main := (
		( '1' 'hello' ) |
		( '2' "hello" ) |
		( '3' /hello/ ) |
		( '4' 'hello'i ) |
		( '5' "hello"i ) |
		( '6' /hello/i )
	) '\n';
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

	if ( cs >= indep_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "1hello\n".to_string() ); }
	unsafe { m( "1HELLO\n".to_string() ); }
	unsafe { m( "1HeLLo\n".to_string() ); }
	unsafe { m( "2hello\n".to_string() ); }
	unsafe { m( "2HELLO\n".to_string() ); }
	unsafe { m( "2HeLLo\n".to_string() ); }
	unsafe { m( "3hello\n".to_string() ); }
	unsafe { m( "3HELLO\n".to_string() ); }
	unsafe { m( "3HeLLo\n".to_string() ); }
	unsafe { m( "4hello\n".to_string() ); }
	unsafe { m( "4HELLO\n".to_string() ); }
	unsafe { m( "4HeLLo\n".to_string() ); }
	unsafe { m( "5hello\n".to_string() ); }
	unsafe { m( "5HELLO\n".to_string() ); }
	unsafe { m( "5HeLLo\n".to_string() ); }
	unsafe { m( "6hello\n".to_string() ); }
	unsafe { m( "6HELLO\n".to_string() ); }
	unsafe { m( "6HeLLo\n".to_string() ); }
}

