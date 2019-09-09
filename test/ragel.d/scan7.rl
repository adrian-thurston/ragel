/*
 * @LANG: c
 */

#include <string.h>
#include <stdio.h>

/*
 * DEMONSTRATS FAILURE TO CALL LEAVING ACTIONS
 * leave on lag not called
 * leave swith3a not called
 */

char * ts ;
char * te ;
int act ;
int token ;

%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
		'a' %{
			printf( "-> leave on last\n" );
		}
		=> {
			printf( "%s", "on last     " );
			if ( p + 1 == te )
				printf( "%s", "yes" );
			printf( "%s", "\n" );
		};

		'b'+ %{
			printf( "-> leave on next\n" );
		}
		=> {
			printf( "%s", "on next     " );
			if ( p + 1 == te )
				printf( "%s", "yes" );
			printf( "%s", "\n" );
		};

		( 'c1' 'dxxx'? ) %{
			printf( "-> leave on lag\n" );
		}
		=> {
			printf( "%s", "on lag      " );
			if ( p + 1 == te )
				printf( "%s", "yes" );
			printf( "%s", "\n" );
		};

		'd1' %{
			printf( "-> leave lm switch1\n" );
		}
		=> {
			printf( "%s", "lm switch1  " );
			if ( p + 1 == te )
				printf( "%s", "yes" );
			printf( "%s", "\n" );
		};
		'd2' %{
			printf( "-> leave lm switch2\n" );
		}
		=> {
			printf( "%s", "lm switch2  " );
			if ( p + 1 == te )
				printf( "%s", "yes" );
			printf( "%s", "\n" );
		};

		[d0-9]+ '.' @{printf("dot\n");} '+' => { printf( "fake out" ); };

		( 'e1' '...'? ) %{printf("-> leave lm switch3a\n"); } => {printf("lm switch3a\n");};
		( 'e2' '...'? ) %{printf("-> leave lm switch3b\n"); } => {printf("lm switch3b\n");};
		[e0-9]+ '...' => {printf("lm switch4\n");};

		'.' => { printf( ".\n" ); };
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
"abbc1d1d2d1..d2..e1.e2....\n",
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

##### OUTPUT #####
-> leave on last
on last     yes
-> leave on next
on next     yes
on lag      yes
-> leave lm switch1
dot
lm switch1  yes
-> leave lm switch2
dot
lm switch2  yes
-> leave lm switch1
dot
lm switch1  yes
.
.
-> leave lm switch2
dot
lm switch2  yes
.
.
lm switch3a
.
-> leave lm switch3b
lm switch3b
.
ACCEPT
