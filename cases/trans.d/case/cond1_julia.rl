//
// @LANG: julia
// @GENERATED: true
//


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

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""
i = 0;
j = 0;
k = 0;

	%% write init;
	%% write exec;

	if ( cs >= foo_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "000abc\n" );
	m( "100abc\n" );
	m( "010abc\n" );
	m( "110abc\n" );
	m( "001abc\n" );
	m( "101abc\n" );
	m( "011abc\n" );
	m( "111abc\n" );
