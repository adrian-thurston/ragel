//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;




%%{
	machine erract;

	action err_start {cout.format( "err_start\n" );
}
	action err_all {cout.format( "err_all\n" );
}
	action err_middle {cout.format( "err_middle\n" );
}
	action err_out {cout.format( "err_out\n" );
}

	action eof_start {cout.format( "eof_start\n" );
}
	action eof_all {cout.format( "eof_all\n" );
}
	action eof_middle {cout.format( "eof_middle\n" );
}
	action eof_out {cout.format( "eof_out\n" );
}

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
		) '\n';
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

	if ( cs >= erract_first_final ) {
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
	m( "x" );
	m( "he" );
	m( "hx" );
	m( "hel" );
	m( "hex" );
	m( "hell" );
	m( "helx" );
	m( "hello" );
	m( "hellx" );
	m( "hello\n" );
	m( "hellox" );
}

main();
