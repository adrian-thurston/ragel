/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>




%%{
	machine eofact;

	action a1 {printf( "%s", "a1\n" );
}
	action a2 {printf( "%s", "a2\n" );
}
	action a3 {printf( "%s", "a3\n" );
}
	action a4 {printf( "%s", "a4\n" );
}


	main := (
		'hello' @eof a1 %eof a2 '\n'? |
		'there' @eof a3 %eof a4
	);

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
	char *eof = pe;
	%% write exec;
}

void finish( )
{
	if ( cs >= eofact_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
"",
"h",
"hell",
"hello",
"hello\n",
"t",
"ther",
"there",
"friend",
};

int inplen = 9;

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

