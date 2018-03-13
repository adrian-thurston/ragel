/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>




%%{
	machine zlen1;
	main := zlen;
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
	if ( cs >= zlen1_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
"",
"x",
};

int inplen = 2;

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

