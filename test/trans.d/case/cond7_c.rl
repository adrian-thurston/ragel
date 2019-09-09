/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>

int i ;
int c ;

%%{
	machine foo;

	action testi {i > 0}
	action inc {i = i - 1;
c = ( int ) ( fc )
;
printf( "%s", "item: " );
printf( "%d", c );
printf( "%s", "\n" );
}

	count = [0-9] @{i = ( int ) ( fc - 48 )
;
printf( "%s", "count: " );
printf( "%d", i );
printf( "%s", "\n" );
};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
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
	if ( cs >= foo_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
"00\n",
"019\n",
"190\n",
"1719\n",
"1040000\n",
"104000a\n",
"104000\n",
};

int inplen = 7;

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

