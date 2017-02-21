#
# @LANG: ruby
# @GENERATED: true
#




%%{
	machine erract;

	action err_start {print( "err_start\n" );
}
	action err_all {print( "err_all\n" );
}
	action err_middle {print( "err_middle\n" );
}
	action err_out {print( "err_out\n" );
}

	action eof_start {print( "eof_start\n" );
}
	action eof_all {print( "eof_all\n" );
}
	action eof_middle {print( "eof_middle\n" );
}
	action eof_out {print( "eof_out\n" );
}

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
		) '\n';
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
	%% write init;
	%% write exec;
	if cs >= erract_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"",
"h",
"x",
"he",
"hx",
"hel",
"hex",
"hell",
"helx",
"hello",
"hellx",
"hello\n",
"hellox",
]

inplen = 13

inp.each { |str| run_machine(str) }

