//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine foo;

	action testi {i > 0}
	action inc {i = i - 1;
c = convert( Int, ( fc ) ) 
;
print( "item: " );
print( c );
print( "\n" );
}

	count = [0-9] @{i = convert( Int, ( fc - 48 ) ) 
;
print( "count: " );
print( i );
print( "\n" );
};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
}%%



%% write data;

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""
i = 0;
c = 0;

	%% write init;
	%% write exec;

	if ( cs >= foo_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "00\n" );
	m( "019\n" );
	m( "190\n" );
	m( "1719\n" );
	m( "1040000\n" );
	m( "104000a\n" );
	m( "104000\n" );
