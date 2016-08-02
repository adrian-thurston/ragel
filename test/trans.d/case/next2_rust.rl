//
// @LANG: rust
// @GENERATED: true
//

static mut target : i32 = 0;
static mut last : i32 = 0;

%%{
	machine next2;

	unused := 'unused';

	one := 'one' @{print!( "{}", "one\n" );
target = fentry(main);
fnext *target;};

	two := 'two' @{print!( "{}", "two\n" );
target = fentry(main);
fnext *target;};

	three := 'three' @{print!( "{}", "three\n" );
target = fentry(main);
fnext *target;};

	main :=  (
		'1' @{target = fentry(one);
fnext *target;last = 1;
} |

		'2' @{target = fentry(two);
fnext *target;last = 2;
} |

		# This one is conditional based on the last.
		'3' @{if ( last == 2 )
{
	target = fentry(three);
fnext *target;
} 
last = 3;
} 'x' |

		'\n'
	)*;
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

	if ( cs >= next2_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "1one3x2two3three\n".to_string() ); }
}

