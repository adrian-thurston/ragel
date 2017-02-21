#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine gotocallret;

	# A reference to a state in an unused action caused a segfault in 5.8. */
	action unusedAction {fentry(garble_line);
}

	action err_garbling_line {print( "error: garbling line\n" );
}
	action goto_main {fnext main;}
	action recovery_failed {print( "error: failed to recover\n" );
}

	# Error machine, consumes to end of 
	# line, then starts the main line over.
	garble_line := ( (any-'\n')*'\n') 
		>err_garbling_line
		@goto_main
		$/recovery_failed;

	action hold_and_return {fhold;fnret;}

	# Look for a string of alphas or of digits, 
	# on anything else, hold the character and return.
	alp_comm := alpha+ $!hold_and_return;
	dig_comm := digit+ $!hold_and_return;

	# Choose which to machine to call into based on the command.
	action comm_arg {if ( comm >= 97 )
	fncall alp_comm;
else 
	fncall dig_comm;
end
}

	# Specifies command string. Note that the arg is left out.
	command = (
		[a-z0-9] @{comm = fc;
} ' ' @comm_arg @{print( "prints\n" );
} '\n'
	) @{print( "correct command\n" );
};

	# Any number of commands. If there is an 
	# error anywhere, garble the line.
	main := command* $!{fhold;fnext garble_line;}; 
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
comm = 1
top = 1
stack = Array.new
	%% write init;
	%% write exec;
	if cs >= gotocallret_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"lkajsdf\n",
"2134\n",
"(\n",
"\n",
"*234234()0909 092 -234aslkf09`1 11\n",
"1\n",
"909\n",
"1 a\n",
"11 1\n",
"a 1\n",
"aa a\n",
"1 1\n",
"1 123456\n",
"a a\n",
"a abcdef\n",
"h",
"a aa1",
]

inplen = 17

inp.each { |str| run_machine(str) }

