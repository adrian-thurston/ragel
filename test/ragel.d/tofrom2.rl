/*
 * @LANG: c
 * 
 * Tests to/from state actions, for comparison against ASM version.
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
	action finish {
		if ( neg != 0 ) {
			value = -1 * value;
		} 
	}

	action print {
		printf( "%d", value );
		printf( "%s", "\n" );
	}

	action tos {
		printf( "to on %d\n", (int)*p );
	}

	action froms {
		printf( "from on %d\n", (int)*p );
	}

	atoi = (
			('-'@see_neg | '+')? (digit @add_digit)+
		) >begin %finish;

	main := ( atoi '\n' @print)
		$to(tos) $from(froms);
}%%


%% write data;
int cs;

void init()
{
	value = 0;
	neg = 0;
	%% write init;
}

void exec( char *data, int len )
{
	char *p = data;
	const char *pe = data + len;
	const char *eof = pe;
	%% write exec;
}

void finish( )
{
	if ( cs < atoi_first_final )
		printf( "-> FAIL\n" );
	else
		printf( "-> ACCEPT\n" );
}

char *inp[] = {
	"1\n",
	"12\n",
	"222222\n",
	"+2123\n",
	"-99\n",
	"-12321\n",
	"213 3213\n",
	"--123\n",
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

##### OUTPUT #####
from on 49
to on 49
from on 10
1
to on 10
from on 0
-> ACCEPT
from on 49
to on 49
from on 50
to on 50
from on 10
12
to on 10
from on 0
-> ACCEPT
from on 50
to on 50
from on 50
to on 50
from on 50
to on 50
from on 50
to on 50
from on 50
to on 50
from on 50
to on 50
from on 10
222222
to on 10
from on 0
-> ACCEPT
from on 43
to on 43
from on 50
to on 50
from on 49
to on 49
from on 50
to on 50
from on 51
to on 51
from on 10
2123
to on 10
from on 0
-> ACCEPT
from on 45
to on 45
from on 57
to on 57
from on 57
to on 57
from on 10
-99
to on 10
from on 0
-> ACCEPT
from on 45
to on 45
from on 49
to on 49
from on 50
to on 50
from on 51
to on 51
from on 50
to on 50
from on 49
to on 49
from on 10
-12321
to on 10
from on 0
-> ACCEPT
from on 50
to on 50
from on 49
to on 49
from on 51
to on 51
from on 32
-> FAIL
from on 45
to on 45
from on 45
-> FAIL
from on 32
-> FAIL
