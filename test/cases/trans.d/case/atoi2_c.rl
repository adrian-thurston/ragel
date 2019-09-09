/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>

int neg ;
int value ;

%%{
	machine state_chart;

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

	atoi = (
		start: (
			'-' @see_neg ->om_num | 
			'+' ->om_num |
			[0-9] @add_digit ->more_nums
		),

		# One or more nums.
		om_num: (
			[0-9] @add_digit ->more_nums
		),

		# Zero ore more nums.
		more_nums: (
			[0-9] @add_digit ->more_nums |
			'' -> final
		)
	) >begin %finish;

	action oneof {printf( "%d", value );
printf( "%s", "\n" );
}
	main := ( atoi '\n' @oneof )*;
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
	if ( cs >= state_chart_first_final )
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

