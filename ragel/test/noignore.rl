/*
 * @LANG: c
 *
 * Tests the no-ignore property following ragel section close. Can't be
 * garbling up whitespace or ragel comments( C defines ).
 */

#include <string.h>
#include <stdio.h>

%%{
	machine atoi;

	main := 'hello\n' @{ printf( "hello\n" ); };
}%%

#define DEF 1

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
	if ( cs >= atoi_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
	"hello\n",
};

int inplen = 9;

int main( )
{
#ifndef DEF
	printf("DEF not defined -- noignore problem\n" );
#else
	int i;
	for ( i = 0; i < inplen; i++ ) {
		init();
		exec( inp[i], strlen(inp[i]) );
		finish();
	}
#endif
	return 0;
}

##### OUTPUT #####
hello
ACCEPT
