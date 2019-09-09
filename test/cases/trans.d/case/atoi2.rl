/*
 * @LANG: indep
 * This implementes an atoi machine using the statechart paradigm.
 */
bool neg;
int value;

value = 0;
neg = false;

%%{
	machine state_chart;

	action begin {
		neg = false;
		value = 0;
	}

	action see_neg {
		neg = true;
	}

	action add_digit { 
		value = value * 10 + <int>(fc - 48);
	}

	action finish {
		if ( neg != 0 ) {
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

	action oneof { print_int value; print_str "\n"; }
	main := ( atoi '\n' @oneof )*;
}%%

##### INPUT #####
"1\n"
"12\n"
"222222\n"
"+2123\n"
"213 3213\n"
"-12321\n"
"--123\n"
"-99\n"
" -3000\n"
##### OUTPUT #####
1
ACCEPT
12
ACCEPT
222222
ACCEPT
2123
ACCEPT
FAIL
-12321
ACCEPT
FAIL
-99
ACCEPT
FAIL
