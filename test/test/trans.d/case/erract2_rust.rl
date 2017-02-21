//
// @LANG: rust
// @GENERATED: true
//




%%{
	machine erract;

	action err_start {print!( "{}", "err_start\n" );
}
	action err_all {print!( "{}", "err_all\n" );
}
	action err_middle {print!( "{}", "err_middle\n" );
}
	action err_out {print!( "{}", "err_out\n" );
}

	action eof_start {print!( "{}", "eof_start\n" );
}
	action eof_all {print!( "{}", "eof_all\n" );
}
	action eof_middle {print!( "{}", "eof_middle\n" );
}
	action eof_out {print!( "{}", "eof_out\n" );
}

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
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

	if ( cs >= erract_first_final ) {
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
	unsafe { m( "x".to_string() ); }
	unsafe { m( "he".to_string() ); }
	unsafe { m( "hx".to_string() ); }
	unsafe { m( "hel".to_string() ); }
	unsafe { m( "hex".to_string() ); }
	unsafe { m( "hell".to_string() ); }
	unsafe { m( "helx".to_string() ); }
	unsafe { m( "hello".to_string() ); }
	unsafe { m( "hellx".to_string() ); }
	unsafe { m( "hello\n".to_string() ); }
	unsafe { m( "hellox".to_string() ); }
}

