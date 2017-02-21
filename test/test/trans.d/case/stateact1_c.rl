/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>




%%{
	machine state_act;

	action a1 {printf( "%s", "a1\n" );
}
	action a2 {printf( "%s", "a2\n" );
}
	action b1 {printf( "%s", "b1\n" );
}
	action b2 {printf( "%s", "b2\n" );
}
	action c1 {printf( "%s", "c1\n" );
}
	action c2 {printf( "%s", "c2\n" );
}
	action next_again {fnext again;}

	hi = 'hi';
	line = again: 
			hi 
				>to b1 
				>from b2 
			'\n' 
				>to c1 
				>from c2 
				@next_again;
		 
	main := line*
			>to a1 
			>from a2;
}%%



%% write data;
int cs;
int blen;
char buffer[1024];

void init()
{
	%% write init;
}

void exec( char *data, int len )
{
	char *p = data;
	char *pe = data + len;
	%% write exec;
}

void finish( )
{
	if ( cs >= state_act_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
"hi\nhi\n",
};

int inplen = 1;

int main( )
{
	int i;
	for ( i = 0; i < inplen; i++ ) {
		init();
		exec( inp[i], strlen(inp[i]) );
		finish();
	}
	return 0;
}

