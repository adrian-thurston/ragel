//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine state_chart;

	action begin {neg = 0;
value = 0;
}

	action see_neg {neg = 1;
}

	action add_digit {value = value * 10 + convert( Int, ( fc - 48 ) ) 
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

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""
neg = 0;
value = 0;
value = 0;
neg = 0;

	%% write init;
	%% write exec;

	if ( cs >= state_chart_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "1\n" );
	m( "12\n" );
	m( "222222\n" );
	m( "+2123\n" );
	m( "213 3213\n" );
	m( "-12321\n" );
	m( "--123\n" );
	m( "-99\n" );
	m( " -3000\n" );
