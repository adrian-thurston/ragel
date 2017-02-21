/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>



%%{
	machine indep;

	main := (
		( '1' 'hello' ) |
		( '2' "hello" ) |
		( '3' /hello/ ) |
		( '4' 'hello'i ) |
		( '5' "hello"i ) |
		( '6' /hello/i )
	) '\n';
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
	if ( cs >= indep_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
"1hello\n",
"1HELLO\n",
"1HeLLo\n",
"2hello\n",
"2HELLO\n",
"2HeLLo\n",
"3hello\n",
"3HELLO\n",
"3HeLLo\n",
"4hello\n",
"4HELLO\n",
"4HeLLo\n",
"5hello\n",
"5HELLO\n",
"5HeLLo\n",
"6hello\n",
"6HELLO\n",
"6HeLLo\n",
};

int inplen = 18;

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

