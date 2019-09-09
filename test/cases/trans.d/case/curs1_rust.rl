//
// @LANG: rust
// @GENERATED: true
//

static mut return_to : i32 = 0;

%%{
	machine curs1;

	unused := 'unused';

	one := 'one' @{print!( "{}", "one\n" );
fnext *return_to;};

	two := 'two' @{print!( "{}", "two\n" );
fnext *return_to;};

	main := 
		'1' @{return_to = fcurs;
fnext one;}
	|	'2' @{return_to = fcurs;
fnext two;}
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

	if ( cs >= curs1_first_final ) {
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

