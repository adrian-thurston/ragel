//
// @LANG: rust
// @GENERATED: true
//

static mut i : i8 = 0;
static mut j : i8 = 0;
static mut k : i8 = 0;

%%{
	machine foo;

	action c1 {i != 0}
	action c2 {j != 0}
	action c3 {k != 0}
	action one {print!( "{}", "  one\n" );
}
	action two {print!( "{}", "  two\n" );
}
	action three {print!( "{}", "  three\n" );
}

	action seti {if ( fc == 48 )
{
	i = 0;

} 
else {
	i = 1;

}
}
	action setj {if ( fc == 48 )
{
	j = 0;

} 
else {
	j = 1;

}
}
	action setk {if ( fc == 48 )
{
	k = 0;

} 
else {
	k = 1;

}
}

	action break {fnbreak;}

	one = 'a' 'b' when c1 'c' @one;
	two = 'a'* 'b' when c2 'c' @two;
	three = 'a'+ 'b' when c3 'c' @three;

	main := 
		[01] @seti
		[01] @setj
		[01] @setk
		( one | two | three ) '\n' @break;
	
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

	if ( cs >= foo_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "000abc\n".to_string() ); }
	unsafe { m( "100abc\n".to_string() ); }
	unsafe { m( "010abc\n".to_string() ); }
	unsafe { m( "110abc\n".to_string() ); }
	unsafe { m( "001abc\n".to_string() ); }
	unsafe { m( "101abc\n".to_string() ); }
	unsafe { m( "011abc\n".to_string() ); }
	unsafe { m( "111abc\n".to_string() ); }
}

