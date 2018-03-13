//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

int top;
array[int] stack = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
int target;

%%{
	machine ncall1;

	unused := 'unused';

	one := 'one' @{cout.format( "one\n" );
fnret;};

	two := 'two' @{cout.format( "two\n" );
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

void m( String s )
{
	byteptr data = s.buffer;
	int p = 0;
	int pe = s.size;
	int cs;
	String buffer;

	%% write init;
	%% write exec;

	if ( cs >= ncall1_first_final ) {
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
