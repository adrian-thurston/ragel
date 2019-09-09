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
static mut val : i32 = 0;

%%{
	machine GotoCallRet;

	sp = ' ';

	handle := any @{print!( "{}", "handle " );
fhold;if ( val == 1 )
{
	fnext *fentry(one);
} 
if ( val == 2 )
{
	fnext *fentry(two);
} 
if ( val == 3 )
{
	fnext main;
} 
};

	one := |*
		'{' => {print!( "{}", "{ " );
fcall *fentry(one);};
		"[" => {print!( "{}", "[ " );
fcall *fentry(two);};
		"}" sp* => {print!( "{}", "} " );
fret;};
		[a-z]+ => {print!( "{}", "word " );
val = 1;
fgoto *fentry(handle);};
		' ' => {print!( "{}", "space " );
};
	*|;

	two := |*
		'{' => {print!( "{}", "{ " );
fcall *fentry(one);};
		"[" => {print!( "{}", "[ " );
fcall *fentry(two);};
		']' sp* => {print!( "{}", "] " );
fret;};
		[a-z]+ => {print!( "{}", "word " );
val = 2;
fgoto *fentry(handle);};
		' ' => {print!( "{}", "space " );
};
	*|;

	main := |* 
		'{' => {print!( "{}", "{ " );
fcall one;};
		"[" => {print!( "{}", "[ " );
fcall two;};
		[a-z]+ => {print!( "{}", "word " );
val = 3;
fgoto handle;};
		[a-z] ' foil' => {print!( "{}", "this is the foil" );
};
		' ' => {print!( "{}", "space " );
};
		'\n';
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

	if ( cs >= GotoCallRet_first_final ) {
		println!( "ACCEPT" );
	}
	else {
		println!( "FAIL" );
	}
}

fn main()
{
	unsafe { m( "{a{b[c d]d}c}\n".to_string() ); }
	unsafe { m( "[a{b[c d]d}c}\n".to_string() ); }
	unsafe { m( "[a[b]c]d{ef{g{h}i}j}l\n".to_string() ); }
	unsafe { m( "{{[]}}\n".to_string() ); }
	unsafe { m( "a b c\n".to_string() ); }
	unsafe { m( "{a b c}\n".to_string() ); }
	unsafe { m( "[a b c]\n".to_string() ); }
	unsafe { m( "{]\n".to_string() ); }
	unsafe { m( "{{}\n".to_string() ); }
	unsafe { m( "[[[[[[]]]]]]\n".to_string() ); }
	unsafe { m( "[[[[[[]]}]]]\n".to_string() ); }
}

