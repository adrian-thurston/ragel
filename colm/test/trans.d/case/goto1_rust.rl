//
// @LANG: rust
// @GENERATED: true
//

static mut target : i32 = 0;

%%{
	machine goto1;

	unused := 'unused';

	one := 'one' @{print!( "{}", "one\n" );
target = fentry(main);
fgoto *target;};

	two := 'two' @{print!( "{}", "two\n" );
target = fentry(main);
fgoto *target;};

	main := 
		'1' @{target = fentry(one);
fgoto *target;}
	|	'2' @{target = fentry(two);
fgoto *target;}
	|	'\n';
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

	if ( cs >= goto1_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "1one2two1one\n".to_string() ); }
}

