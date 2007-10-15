/*
 * @LANG: c
 * Test growable stack.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

%%{
	machine recdescent;

	prepush { 
		if ( top == stack_size ) {
			printf( "growing stack\n" );
			stack_size = top * 2;
			stack = (int*)realloc( stack, sizeof(int)*stack_size );
		}
	}

	postpop { 
		if ( stack_size > (top * 4) ) {
			stack_size = top * 2;
			stack = (int*)realloc( stack, sizeof(int)*stack_size );
			printf( "shrinking stack\n" );
		}
	}

	action item_start { item = p; }

	action item_finish
	{
		printf( "item: " );
		fwrite( item, 1, p-item, stdout );
		printf( "\n" );
	}

	action call_main
	{
		printf( "calling main\n" );
		fcall main;
	}

	action return_main
	{
		if ( top == 0 ) {
			printf( "STRAY CLOSE\n" );
			fbreak;
		}

		printf( "returning from main\n" );
		fhold;
		fret;
	}

	id = [a-zA-Z_]+;
	number = [0-9]+;
	ws = [ \t\n]+;

	main := ( 
		ws |
		( number | id ) >item_start %item_finish |

		'{' @call_main '}' |

		'}' @return_main
	)**;
}%%

%% write data;

void test( char *buf )
{
	int cs;
	int *stack;
	int top, stack_size;
	char *p, *pe, *eof, *item = 0;

	int len = strlen( buf );

	%% write init;

	stack_size = 1;
	stack = (int*)malloc( sizeof(int) * stack_size );

	p = buf;
	pe = buf + len;
	eof = pe;

	%% write exec;

	if ( cs == recdescent_error ) {
		/* Machine failed before finding a token. */
		printf( "PARSE ERROR\n" );
	}
}

int main()
{
	test( "88 foo { 99 {{{{}}}}{ } }");
	test( "76 } sadf");
	return 0;
}

#ifdef _____OUTPUT_____
item: 88
item: foo
calling main
item: 99
calling main
growing stack
calling main
growing stack
calling main
calling main
growing stack
returning from main
returning from main
returning from main
returning from main
shrinking stack
calling main
returning from main
returning from main
shrinking stack
item: 76
STRAY CLOSE
#endif
