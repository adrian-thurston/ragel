/*
 * @LANG: indep
 * This implementes an atoi machine using the statechart paradigm.
 */
bool neg;
int val;
%%
val = 0;
neg = false;
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
		val = val * 10 + <int>(fc - 48);
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

	action oneof { printi val; prints "\n"; }
	main := ( atoi '\n' @oneof )*;
}%%
/* _____INPUT_____
"1\n"
"12\n"
"222222\n"
"+2123\n"
"213 3213\n"
"-12321\n"
"--123\n"
"-99\n"
" -3000\n"
_____INPUT_____ */

/* _____OUTPUT_____
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
_____OUTPUT_____ */
