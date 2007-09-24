#
# @LANG: ruby
#

%%{
	machine test;

	export c1 = 'c';
	export c2 = 'z';
	export c3 = 't';

	commands := (
		c1 . digit* '\n' @{ puts "c1"; } |
		c2 . alpha* '\n' @{ puts "c2"; }|
		c3 . '.'* '\n' @{ puts "c3"; }
	)*;
			
	other := any*;
}%%

%% write exports;
%% write data;

def run_machine( data )
	p = 0;
	pe = data.length
	cs = test_en_commands
	val = 0;
	neg = false;

	%% write init nocs;
	%% write exec;
	if  cs >= test_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
		test_ex_c1, ?1, ?2, ?\n, 
		test_ex_c2, ?a, ?b, ?\n, 
		test_ex_c3, ?., ?., ?\n
]

run_machine( inp );

=begin _____OUTPUT_____
c1
c2
c3
ACCEPT
=end _____OUTPUT_____
