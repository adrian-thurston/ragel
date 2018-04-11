//
// @LANG: rust
// @GENERATED: true
//




%%{
	machine state_act;

	action a1 {print!( "{}", "a1\n" );
}
	action a2 {print!( "{}", "a2\n" );
}
	action b1 {print!( "{}", "b1\n" );
}
	action b2 {print!( "{}", "b2\n" );
}
	action c1 {print!( "{}", "c1\n" );
}
	action c2 {print!( "{}", "c2\n" );
}
	action next_again {fnext again;}

	hi = 'hi';
	line = again: 
			hi 
				>to b1 
				>from b2 
			'\n' 
				>to c1 
				>from c2 
				@next_again;
		 
	main := line*
			>to a1 
			>from a2;
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

	if ( cs >= state_act_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "hi\nhi\n".to_string() ); }
}

