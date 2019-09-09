//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

byte comm;
int top;
array[int] stack = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
int
 ts;
int
 te;
int act;
int value;

%%{
	machine patact;

	other := |* 
		[a-z]+ => {cout.format( "word\n" );
};
		[0-9]+ => {cout.format( "num\n" );
};
		[\n ] => {cout.format( "space\n" );
};
	*|;

	exec_test := |* 
		[a-z]+ => {cout.format( "word (w/lbh)\n" );
fexec te-1;fgoto other;};
		[a-z]+ ' foil' => {cout.format( "word (c/lbh)\n" );
};
		[\n ] => {cout.format( "space\n" );
};
		'22' => {cout.format( "num (w/switch)\n" );
};
		[0-9]+ => {cout.format( "num (w/switch)\n" );
fexec te-1;fgoto other;};
		[0-9]+ ' foil' => {cout.format( "num (c/switch)\n" );
};
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => {cout.format( "in semi\n" );
fgoto main;};
	*|;

	main := |* 
		[a-z]+ => {cout.format( "word (w/lbh)\n" );
fhold;fgoto other;};
		[a-z]+ ' foil' => {cout.format( "word (c/lbh)\n" );
};
		[\n ] => {cout.format( "space\n" );
};
		'22' => {cout.format( "num (w/switch)\n" );
};
		[0-9]+ => {cout.format( "num (w/switch)\n" );
fhold;fgoto other;};
		[0-9]+ ' foil' => {cout.format( "num (c/switch)\n" );
};
		';' => {cout.format( "going to semi\n" );
fhold;fgoto semi;};
		'!' => {cout.format( "immdiate\n" );
fgoto exec_test;};
	*|;
}%%



%% write data;

void m( String s )
{
	byteptr data = s.buffer;
	int p = 0;
	int pe = s.size;
	int cs;
	String buffer;
	int eof = pe;

	%% write init;
	%% write exec;

	if ( cs >= patact_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "abcd foix\n" );
	m( "abcd\nanother\n" );
	m( "123 foix\n" );
	m( "!abcd foix\n" );
	m( "!abcd\nanother\n" );
	m( "!123 foix\n" );
	m( ";" );
}

main();
