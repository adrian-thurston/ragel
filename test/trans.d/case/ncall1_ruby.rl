#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine ncall1;

	unused := 'unused';

	one := 'one' @{print( "one\n" );
fnret;};

	two := 'two' @{print( "two\n" );
fnret;};

	main := (
			'1' @{target = fentry(one);
fncall *target;}
		|	'2' @{target = fentry(two);
fncall *target;}
		|	'\n'
	)*;
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
top = 1
stack = Array.new
target = 1
	%% write init;
	%% write exec;
	if cs >= ncall1_first_final
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

