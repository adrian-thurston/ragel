/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>

int neg ;
int value ;

%%{
	machine atoi;

	action begin {neg = 0;
value = 0;
}

	action see_neg {neg = 1;
}

	action add_digit {value = value * 10 + ( int ) ( fc - 48 )
;
}

	action finish {if ( neg != 0 )
{
	value = -1 * value;

} 
}
	action print {printf( "%d", value );
printf( "%s", "\n" );
}

	atoi = (
		('-'@see_neg | '+')? (digit @add_digit)+
	) >begin %finish;

	main := atoi '\n' @print;
}%%



%% write data;
int cs;
int blen;
char buffer[1024];

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

