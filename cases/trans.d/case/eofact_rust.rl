//
// @LANG: rust
// @GENERATED: true
//




%%{
	machine eofact;

	action a1 {print!( "{}", "a1\n" );
}
	action a2 {print!( "{}", "a2\n" );
}
	action a3 {print!( "{}", "a3\n" );
}
	action a4 {print!( "{}", "a4\n" );
}


	main := (
		'hello' @eof a1 %eof a2 '\n'? |
		'there' @eof a3 %eof a4
	);

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

	if ( cs >= eofact_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "".to_string() ); }
	unsafe { m( "h".to_string() ); }
	unsafe { m( "hell".to_string() ); }
	unsafe { m( "hello".to_string() ); }
	unsafe { m( "hello\n".to_string() ); }
	unsafe { m( "t".to_string() ); }
	unsafe { m( "ther".to_string() ); }
	unsafe { m( "there".to_string() ); }
	unsafe { m( "friend".to_string() ); }
}

