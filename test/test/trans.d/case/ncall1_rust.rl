//
// @LANG: rust
// @GENERATED: true
//

static mut top : i32 = 0;
static mut stack : [ i32 ; 32] = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
static mut target : i32 = 0;

%%{
	machine ncall1;

	unused := 'unused';

	one := 'one' @{print!( "{}", "one\n" );
fnret;};

	two := 'two' @{print!( "{}", "two\n" );
fnret;};

	main := (
			'1' @{target = fentry(one);
fncall *target;}
		|	'2' @{target = fentry(two);
fncall *target;}
		|	'\n'
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

	if ( cs >= ncall1_first_final ) {
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

