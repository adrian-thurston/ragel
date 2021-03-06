/*
 * @LANG: c
 * @FILTER: LC_ALL=C sort
 */

#include <string.h>
#include <stdio.h>

int neg ;
long value ;

struct nfa_bp_rec
{
	long state;
	char *p;
	int pop;
};

struct nfa_bp_rec nfa_bp[1024];
long nfa_len = 0;
long nfa_count = 0;

%%{
	machine atoi;

	action begin {neg = 0;
		value = 0;
	}

	action see_neg {
		neg = 1;
	}

	action add_digit {
		value = value * 10 + ( int ) ( fc - 48 );
	}

	action finish {
		if ( neg ) { value = -1 * value; }
	}

	action print1 {
		printf( "%ld", value );
		printf( "%s", "\n" );
		fnext *atoi_error;
	}

	action print2 {
		printf( "saw 80808080\n" );
		fnext *atoi_error;
	}

	atoi = (
		('-'@see_neg | '+')? (digit @add_digit)+
	) >begin %finish;

	main1 = atoi '\n' @print1;
	main2 = [0-9]* '00000000' [0-9]* '\n' @print2;

	main |= (5, 0) main1 | main2;
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
	char *eof = pe;
	%% write exec;
}

void finish( )
{
	if ( cs >= atoi_first_final )
		printf( "-> accept\n\n" );
	else
		printf( "-> fail\n\n" );
}

char *inp[] = {
	"1\n",
	"12\n",
	"1002000000002\n",
	"222222\n",
	"+2123\n",
	"-12321\n",
	"-99\n",
	"213 3213\n",
	"--123\n",
	" -3000\n",
};

int inplen = 10;

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










-12321
-99
-> fail
-> fail
-> fail
-> fail
-> fail
-> fail
-> fail
-> fail
-> fail
-> fail
1
1002000000002
12
2123
222222
saw 80808080
