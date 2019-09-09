//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

int target;

%%{
	machine next1;

	unused := 'unused';

	one := 'one' @{cout.format( "one\n" );
target = fentry(main);
fnext *target;};

	two := 'two' @{cout.format( "two\n" );
target = fentry(main);
fnext *target;};

	main := 
		'1' @{target = fentry(one);
fnext *target;}
	|	'2' @{target = fentry(two);
fnext *target;}
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

	if ( cs >= next1_first_final ) {
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
