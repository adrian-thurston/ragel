//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;




%%{
	machine eofact;

	action a1 {cout.format( "a1\n" );
}
	action a2 {cout.format( "a2\n" );
}
	action a3 {cout.format( "a3\n" );
}
	action a4 {cout.format( "a4\n" );
}


	main := (
		'hello' @eof a1 %eof a2 '\n'? |
		'there' @eof a3 %eof a4
	);

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

	if ( cs >= eofact_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "" );
	m( "h" );
	m( "hell" );
	m( "hello" );
	m( "hello\n" );
	m( "t" );
	m( "ther" );
	m( "there" );
	m( "friend" );
}

main();
