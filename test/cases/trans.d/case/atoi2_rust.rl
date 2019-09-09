//
// @LANG: rust
// @GENERATED: true
//

static mut neg : i8 = 0;
static mut value : i32 = 0;

%%{
	machine state_chart;

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

	atoi = (
		start: (
			'-' @see_neg ->om_num | 
			'+' ->om_num |
			[0-9] @add_digit ->more_nums
		),

		# One or more nums.
		om_num: (
			[0-9] @add_digit ->more_nums
		),

		# Zero ore more nums.
		more_nums: (
			[0-9] @add_digit ->more_nums |
			'' -> final
		)
	) >begin %finish;

	action oneof {print!( "{}", value );
print!( "{}", "\n" );
}
	main := ( atoi '\n' @oneof )*;
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

	if ( cs >= state_chart_first_final ) {
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

