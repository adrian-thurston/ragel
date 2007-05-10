#
# @LANG: ruby
#
# Test the host language scanning for ruby.
#

%%{
	machine erract9;

	action on_char  { print("char: ", data[p..p], "\n"); }
	action on_err   { print("err: ", data[p..p], "\n"); }
	action to_state { print("to state: " , data[p..p], "\n"); }

	main := 'heXXX' $on_char $err(on_err) $to(to_state);
}%%

%% write data;

def run_machine( data )
	p = 0;
	pe = data.length
	cs = 0

	%% write init;
	%% write exec;

	print("rest: " , data[p..p+2], "\n")
end

inp = [
		"hello\n",
]

inp.each { |str| run_machine(str) }

=begin _____OUTPUT_____
char: h
to state: h
char: e
to state: e
err: l
rest: llo
=end _____OUTPUT_____
