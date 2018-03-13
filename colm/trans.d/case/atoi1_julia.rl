//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine atoi;

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
	action print {print( value );
print( "\n" );
}

	atoi = (
		('-'@see_neg | '+')? (digit @add_digit)+
	) >begin %finish;

	main := atoi '\n' @print;
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

	if ( cs >= atoi_first_final )
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
