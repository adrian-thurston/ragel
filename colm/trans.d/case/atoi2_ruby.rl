#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine state_chart;

	action begin {neg = 0;
value = 0;
}

	action see_neg {neg = 1;
}

	action add_digit {value = value * 10 + ( fc - 48 )
;
}

	action finish {if ( neg != 0 )
	value = -1 * value;

end
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

	action oneof {print( value );
print( "\n" );
}
	main := ( atoi '\n' @oneof )*;
}%%



%% write data;

def run_machine( data )
	p = 0
	pe = data.length
	eof = data.length
	cs = 0;
	_m = 
	_a = 
	buffer = Array.new
	blen = 0
neg = 1
value = 1
value = 0;
neg = 0;
	%% write init;
	%% write exec;
	if cs >= state_chart_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"1\n",
"12\n",
"222222\n",
"+2123\n",
"213 3213\n",
"-12321\n",
"--123\n",
"-99\n",
" -3000\n",
]

inplen = 9

inp.each { |str| run_machine(str) }

