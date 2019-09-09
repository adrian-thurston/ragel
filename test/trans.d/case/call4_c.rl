/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>

int target ;
int top ;
int stack [32];

%%{
	machine call4;

	unused := 'unused';

	one := 'one' @{printf( "%s", "one\n" );
fret;};

	two := 'two' @{printf( "%s", "two\n" );
fret;};

	main := (
			'1' @{target = fentry(one);
fcall *target;}
		|	'2' @{target = fentry(two);
fcall *target;}
		|	'\n'
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

void exec( char *data, int len )
{
	char *p = data;
	char *pe = data + len;
	%% write exec;
}

void finish( )
{
	if ( cs >= call4_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
"1one2two1one\n",
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

