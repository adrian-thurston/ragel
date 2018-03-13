//
// @LANG: rust
// @GENERATED: true
//

static mut comm : u8 = 0;
static mut top : i32 = 0;
static mut stack : [ i32 ; 32] = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];

%%{
	machine GotoCallRet;

	# A reference to a state in an unused action caused a segfault in 5.8. */
	action unusedAction {fentry(garble_line);
}

	action err_garbling_line {print!( "{}", "error: garbling line\n" );
}
	action goto_main {fgoto main;}
	action recovery_failed {print!( "{}", "error: failed to recover\n" );
}

	# Error machine, consumes to end of 
	# line, then starts the main line over.
	garble_line := ( (any-'\n')*'\n') 
		>err_garbling_line
		@goto_main
		$/recovery_failed;

	action hold_and_return {fhold;fret;}

	# Look for a string of alphas or of digits, 
	# on anything else, hold the character and return.
	alp_comm := alpha+ $!hold_and_return;
	dig_comm := digit+ $!hold_and_return;

	# Choose which to machine to call into based on the command.
	action comm_arg {if ( comm >= 'a' )
{
	fcall alp_comm;
} 
else {
	fcall dig_comm;
}
}

	# Specifies command string. Note that the arg is left out.
	command = (
		[a-z0-9] @{comm = fc;
} ' ' @comm_arg '\n'
	) @{print!( "{}", "correct command\n" );
};

	# Any number of commands. If there is an 
	# error anywhere, garble the line.
	main := command* $!{fhold;fgoto garble_line;}; 
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

	if ( cs >= GotoCallRet_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "lkajsdf\n".to_string() ); }
	unsafe { m( "2134\n".to_string() ); }
	unsafe { m( "(\n".to_string() ); }
	unsafe { m( "\n".to_string() ); }
	unsafe { m( "*234234()0909 092 -234aslkf09`1 11\n".to_string() ); }
	unsafe { m( "1\n".to_string() ); }
	unsafe { m( "909\n".to_string() ); }
	unsafe { m( "1 a\n".to_string() ); }
	unsafe { m( "11 1\n".to_string() ); }
	unsafe { m( "a 1\n".to_string() ); }
	unsafe { m( "aa a\n".to_string() ); }
	unsafe { m( "1 1\n".to_string() ); }
	unsafe { m( "1 123456\n".to_string() ); }
	unsafe { m( "a a\n".to_string() ); }
	unsafe { m( "a abcdef\n".to_string() ); }
	unsafe { m( "h".to_string() ); }
	unsafe { m( "a aa1".to_string() ); }
}

