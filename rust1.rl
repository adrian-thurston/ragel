//
// @LANG: rust
//

static neg: bool = false;
static value: i32 = 0;

%%{
	machine atoi;

	main := '-'? [0-9]+ '.' @{
		println!( "match" );
	};
}%%

%% write data;

fn m( s: String )
{
	let data: &[u8] = s.as_bytes();
	let mut p = 0;
	let mut pe = s.len();
	let mut cs: i32 = 0;

	%% write init;
	%% write exec;
}

fn main()
{
	m( "-99.".to_string() );
	m( "100.".to_string() );
	m( "100x.".to_string() );
	m( "1000.".to_string() );
}

##### OUTPUT #####
match
match
match
