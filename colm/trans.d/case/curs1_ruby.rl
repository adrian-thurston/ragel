#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine curs1;

	unused := 'unused';

	one := 'one' @{print( "one\n" );
fnext *return_to;};

	two := 'two' @{print( "two\n" );
fnext *return_to;};

	main := 
		'1' @{return_to = fcurs;
fnext one;}
	|	'2' @{return_to = fcurs;
fnext two;}
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
return_to = 1
	%% write init;
	%% write exec;
	if cs >= curs1_first_final
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

