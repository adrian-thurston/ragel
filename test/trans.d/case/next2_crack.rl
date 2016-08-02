//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

int target;
int last;

%%{
	machine next2;

	unused := 'unused';

	one := 'one' @{cout.format( "one\n" );
target = fentry(main);
fnext *target;};

	two := 'two' @{cout.format( "two\n" );
target = fentry(main);
fnext *target;};

	three := 'three' @{cout.format( "three\n" );
target = fentry(main);
fnext *target;};

	main :=  (
		'1' @{target = fentry(one);
fnext *target;last = 1;
} |

		'2' @{target = fentry(two);
fnext *target;last = 2;
} |

		# This one is conditional based on the last.
		'3' @{if ( last == 2 )
{
	target = fentry(three);
fnext *target;
} 
last = 3;
} 'x' |

		'\n'
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

	if ( cs >= next2_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "1one3x2two3three\n" );
}

main();
