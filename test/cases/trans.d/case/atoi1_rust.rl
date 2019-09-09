//
// @LANG: rust
// @GENERATED: true
//

static mut neg : i8 = 0;
static mut value : i32 = 0;

%%{
	machine atoi;

	action begin {neg = 0;
value = 0;
}

	action see_neg {neg = 1;
}

	action add_digit {value = value * 10 + ( ( fc - 48 ) as i32 ) 
;
}

	action finish {if ( neg != 0 )
{
	value = -1 * value;

} 
}
	action print {print!( "{}", value );
print!( "{}", "\n" );
}

	atoi = (
		('-'@see_neg | '+')? (digit @add_digit)+
	) >begin %finish;

	main := atoi '\n' @print;
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
value = 0;
neg = 0;

	%% write init;
	%% write exec;

	if ( cs >= atoi_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "1\n".to_string() ); }
	unsafe { m( "12\n".to_string() ); }
	unsafe { m( "222222\n".to_string() ); }
	unsafe { m( "+2123\n".to_string() ); }
	unsafe { m( "213 3213\n".to_string() ); }
	unsafe { m( "-12321\n".to_string() ); }
	unsafe { m( "--123\n".to_string() ); }
	unsafe { m( "-99\n".to_string() ); }
	unsafe { m( " -3000\n".to_string() ); }
}

