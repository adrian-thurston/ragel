#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine atoi;

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
	action print {print( value );
print( "\n" );
}

	atoi = (
		('-'@see_neg | '+')? (digit @add_digit)+
	) >begin %finish;

	main := atoi '\n' @print;
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
	if cs >= atoi_first_final
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

