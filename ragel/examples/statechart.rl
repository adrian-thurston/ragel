/*
 * Demonstrate the use of labels, the epsilon operator, and the join operator
 * for creating machines using the named state and transition list paradigm.
 * This implementes the same machine as the atoi example.
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace std;

struct StateChart
{
	bool neg;
	int val;
	int cs;

	int init( );
	int execute( const char *data, int len );
	int finish( );
};

%%{
	machine StateChart;

	action begin {
		neg = false;
		val = 0;
	}

	action see_neg {
		neg = true;
	}

	action add_digit { 
		val = val * 10 + (fc - '0');
	}

	action finish {
		if ( neg )
			val = -1 * val;
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

	main := ( atoi '\n' @{ cout << val << endl; } )*;
}%%

%% write data;

int StateChart::init( )
{
	neg = false;
	val = false;
	%% write init;
	return 1;
}

int StateChart::execute( const char *data, int len )
{
	const char *p = data;
	const char *pe = data + len;

	%% write exec;

	if ( cs == StateChart_error )
		return -1;
	if ( cs >= StateChart_first_final )
		return 1;
	return 0;
}

int StateChart::finish( )
{
	if ( cs == StateChart_error )
		return -1;
	if ( cs >= StateChart_first_final )
		return 1;
	return 0;
}


#define BUFSIZE 1024

int main()
{
	char buf[BUFSIZE];

	StateChart atoi;
	atoi.init();
	while ( fgets( buf, sizeof(buf), stdin ) != 0 ) {
		atoi.execute( buf, strlen(buf) );
	}
	if ( atoi.finish() <= 0 )
		cerr << "statechart: error: parsing input" << endl;
	return 0;
}
