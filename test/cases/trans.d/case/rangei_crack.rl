//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;



%%{
	machine rangei;

	main := 
		'a' ../i 'z' .
		'A' ../i 'Z' .
		60 ../i 93 .
		94 ../i 125 .
		86 ../i 101 .
		60 ../i 125
		'';
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

	if ( cs >= rangei_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "AaBbAa" );
	m( "Aa`bAa" );
	m( "AaB@Aa" );
	m( "AaBbMa" );
	m( "AaBbma" );
}

main();
