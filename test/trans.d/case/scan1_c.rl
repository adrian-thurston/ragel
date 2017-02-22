/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>

char * ts ;
char * te ;
int act ;
int token ;

%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
		'a' => {printf( "%s", "on last     " );
if ( p + 1 == te )
{
	printf( "%s", "yes" );

} 
printf( "%s", "\n" );
};

		'b'+ => {printf( "%s", "on next     " );
if ( p + 1 == te )
{
	printf( "%s", "yes" );

} 
printf( "%s", "\n" );
};

		'c1' 'dxxx'? => {printf( "%s", "on lag      " );
if ( p + 1 == te )
{
	printf( "%s", "yes" );

} 
printf( "%s", "\n" );
};

		'd1' => {printf( "%s", "lm switch1  " );
if ( p + 1 == te )
{
	printf( "%s", "yes" );

} 
printf( "%s", "\n" );
};
		'd2' => {printf( "%s", "lm switch2  " );
if ( p + 1 == te )
{
	printf( "%s", "yes" );

} 
printf( "%s", "\n" );
};

		[d0-9]+ '.';

		'\n';
	*|;
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
	if ( cs >= scanner_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
"abbc1d1d2\n",
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

