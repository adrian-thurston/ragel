//
// @LANG: rust
// @GENERATED: true
//

static mut i : i32 = 0;
static mut c : i32 = 0;

%%{
	machine foo;

	action testi {i > 0}
	action inc {i = i - 1;
c = ( ( fc ) as i32 ) 
;
print!( "{}", "item: " );
print!( "{}", c );
print!( "{}", "\n" );
}

	count = [0-9] @{i = ( ( fc - 48 ) as i32 ) 
;
print!( "{}", "count: " );
print!( "{}", i );
print!( "{}", "\n" );
};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
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
	unsafe { m( "00\n".to_string() ); }
	unsafe { m( "019\n".to_string() ); }
	unsafe { m( "190\n".to_string() ); }
	unsafe { m( "1719\n".to_string() ); }
	unsafe { m( "1040000\n".to_string() ); }
	unsafe { m( "104000a\n".to_string() ); }
	unsafe { m( "104000\n".to_string() ); }
}

