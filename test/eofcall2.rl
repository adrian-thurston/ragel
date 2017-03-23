/*
 * @LANG: c
 *
 * Testing fcall * in an EOF action.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int neg;
int value;

%%{
	machine atoi;

	action begin {
		neg = 0;
		value = 0;
	}

	action see_neg
	{
		neg = 1;
	}

	action add_digit
	{
		value = value * 10 + ( int ) ( fc - 48 );
	}

	action finish
	{
		if ( neg != 0 ) {
			value = -1 * value;
		} 
	}

	action print
	{
		printf( "%d", value );
		printf( "%s", "\n" );
	}

	atoi = (
		('-'@see_neg | '+')? (digit @add_digit)+
	) >begin %finish;

	main := atoi '\n' @print %{ printf("goto extra\n"); fcall *fentry(extra); };

	extra := "" %{ printf("done: %d\n", (int)(p-data)); };

}%%

%% write data;
int cs;
int blen;
char buffer[1024];
int top, stack[1];

void init()
{
	value = 0;
	neg = 0;
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
	if ( cs >= atoi_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
	
}

char *inp[] = {
	"1\n",
	"12\n",
	"222222\n",
	"+2123\n",
	"213 3213\n",
	"-12321\n",
	"--123\n",
	"-99\n",
	" -3000\n",
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

#ifdef ___________OUTPUT___________
1
goto extra
done: 2
ACCEPT
12
goto extra
done: 3
ACCEPT
222222
goto extra
done: 7
ACCEPT
2123
goto extra
done: 6
ACCEPT
FAIL
-12321
goto extra
done: 7
ACCEPT
FAIL
-99
goto extra
done: 4
ACCEPT
FAIL
#endif
