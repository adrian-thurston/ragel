//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;




%%{
	machine zlen1;
	main := zlen;
}%%



%% write data;

void m( String s )
{
	byteptr data = s.buffer;
	int p = 0;
	int pe = s.size;
	int cs;
	String buffer;

	%% write init;
	%% write exec;

	if ( cs >= zlen1_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "" );
	m( "x" );
}

main();
