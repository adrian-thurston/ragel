//
// @LANG: rust
// @GENERATED: true
//

static mut comm : u8 = 0;
static mut top : i32 = 0;
static mut stack : [ i32 ; 32] = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
static mut ts : i32
 = 0;
static mut te : i32
 = 0;
static mut act : i32 = 0;
static mut value : i32 = 0;

%%{
	machine patact;

	other := |* 
		[a-z]+ => {print!( "{}", "word\n" );
};
		[0-9]+ => {print!( "{}", "num\n" );
};
		[\n ] => {print!( "{}", "space\n" );
};
	*|;

	exec_test := |* 
		[a-z]+ => {print!( "{}", "word (w/lbh)\n" );
fexec te-1;fgoto other;};
		[a-z]+ ' foil' => {print!( "{}", "word (c/lbh)\n" );
};
		[\n ] => {print!( "{}", "space\n" );
};
		'22' => {print!( "{}", "num (w/switch)\n" );
};
		[0-9]+ => {print!( "{}", "num (w/switch)\n" );
fexec te-1;fgoto other;};
		[0-9]+ ' foil' => {print!( "{}", "num (c/switch)\n" );
};
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => {print!( "{}", "in semi\n" );
fgoto main;};
	*|;

	main := |* 
		[a-z]+ => {print!( "{}", "word (w/lbh)\n" );
fhold;fgoto other;};
		[a-z]+ ' foil' => {print!( "{}", "word (c/lbh)\n" );
};
		[\n ] => {print!( "{}", "space\n" );
};
		'22' => {print!( "{}", "num (w/switch)\n" );
};
		[0-9]+ => {print!( "{}", "num (w/switch)\n" );
fhold;fgoto other;};
		[0-9]+ ' foil' => {print!( "{}", "num (c/switch)\n" );
};
		';' => {print!( "{}", "going to semi\n" );
fhold;fgoto semi;};
		'!' => {print!( "{}", "immdiate\n" );
fgoto exec_test;};
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

	if ( cs >= patact_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "abcd foix\n".to_string() ); }
	unsafe { m( "abcd\nanother\n".to_string() ); }
	unsafe { m( "123 foix\n".to_string() ); }
	unsafe { m( "!abcd foix\n".to_string() ); }
	unsafe { m( "!abcd\nanother\n".to_string() ); }
	unsafe { m( "!123 foix\n".to_string() ); }
	unsafe { m( ";".to_string() ); }
}

