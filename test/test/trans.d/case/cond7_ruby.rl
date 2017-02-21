#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine foo;

	action testi {i > 0}
	action inc {i = i - 1;
c = ( fc )
;
print( "item: " );
print( c );
print( "\n" );
}

	count = [0-9] @{i = ( fc - 48 )
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
c = 1
	%% write init;
	%% write exec;
	if cs >= foo_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"00\n",
"019\n",
"190\n",
"1719\n",
"1040000\n",
"104000a\n",
"104000\n",
]

inplen = 7

inp.each { |str| run_machine(str) }

