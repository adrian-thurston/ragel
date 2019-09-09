#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine foo;

	action c1 {i != 0}
	action c2 {j != 0}
	action c3 {k != 0}
	action one {print( "  one\n" );
}
	action two {print( "  two\n" );
}
	action three {print( "  three\n" );
}

	action seti {if ( fc == 48 )
	i = 0;

else 
	i = 1;

end
}
	action setj {if ( fc == 48 )
	j = 0;

else 
	j = 1;

end
}
	action setk {if ( fc == 48 )
	k = 0;

else 
	k = 1;

end
}

	action break {fnbreak;}

	one = 'a' 'b' when c1 'c' @one;
	two = 'a'* 'b' when c2 'c' @two;
	three = 'a'+ 'b' when c3 'c' @three;

	main := 
		[01] @seti
		[01] @setj
		[01] @setk
		( one | two | three ) '\n' @break;
	
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
i = 1
j = 1
k = 1
	%% write init;
	%% write exec;
	if cs >= foo_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"000abc\n",
"100abc\n",
"010abc\n",
"110abc\n",
"001abc\n",
"101abc\n",
"011abc\n",
"111abc\n",
]

inplen = 8

inp.each { |str| run_machine(str) }

