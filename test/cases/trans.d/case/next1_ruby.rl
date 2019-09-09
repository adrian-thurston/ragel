#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine next1;

	unused := 'unused';

	one := 'one' @{print( "one\n" );
target = fentry(main);
fnext *target;};

	two := 'two' @{print( "two\n" );
target = fentry(main);
fnext *target;};

	main := 
		'1' @{target = fentry(one);
fnext *target;}
	|	'2' @{target = fentry(two);
fnext *target;}
	|	'\n';
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
target = 1
	%% write init;
	%% write exec;
	if cs >= next1_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"1one2two1one\n",
]

inplen = 1

inp.each { |str| run_machine(str) }

