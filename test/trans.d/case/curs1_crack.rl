//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

int return_to;

%%{
	machine curs1;

	unused := 'unused';

	one := 'one' @{cout.format( "one\n" );
fnext *return_to;};

	two := 'two' @{cout.format( "two\n" );
fnext *return_to;};

	main := 
		'1' @{return_to = fcurs;
fnext one;}
	|	'2' @{return_to = fcurs;
fnext two;}
	|	'\n';
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

	if ( cs >= curs1_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "1one2two1one\n" );
}

main();
