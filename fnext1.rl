/*
 * @LANG: c
 *
 * Tests fnext in combination with fbreak.
 */

#include <string.h>
#include <stdio.h>

char comm;
int top;
int stack [32];

%%{
    machine fnext;
	action break {fbreak;}

	main := 'h'   @{ /*h*/  fnext e; fbreak; };
	e    := 'e'   @{ /*e*/  fnext l; }   @{ fbreak; };
	l    := 'll'  @{ /*ll*/ fnext o; }   ${ fbreak; };
	o    := |* 'o' { /*o*/  fnext nl; fbreak; }; *|;
	nl   := '\n'  @{ /*nl*/ fbreak; printf("ACCEPT\n"); };
}%%

int cs;
char *ts, *te;
int act;

%% write data;

void init()
{
	%% write init;
}

void exec( char *data, int len )
{
	char *p = data;
	char *pe = data + len;

	while ( cs != fnext_error && p < pe ) {
		printf( "%c\n", *p );
		%% write exec;
	}
}

void finish( )
{
	if ( cs >= fnext_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
	"hello\n"
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

#ifdef _____OUTPUT_____
h
e
l
l
o


ACCEPT
#endif
