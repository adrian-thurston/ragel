/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class next2
{
int target;
int last;

%%{
	machine next2;

	unused := 'unused';

	one := 'one' @{printf( "%.*s", "one\n" );target = fentry(main);
fnext *target;};

	two := 'two' @{printf( "%.*s", "two\n" );target = fentry(main);
fnext *target;};

	three := 'three' @{printf( "%.*s", "three\n" );target = fentry(main);
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
int cs;
int blen;
char buffer[1024];

void init()
{
	%% write init;
}
void exec( const(char) data[] )
{
	const(char) *p = data.ptr;
	const(char) *pe = data.ptr + data.length;
	char _s[];

	%% write exec;
}

void finish( )
{
	if ( cs >= next2_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"1one3x2two3three\n",
];

int inplen = 1;

}
int main()
{
	next2 m = new next2();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

