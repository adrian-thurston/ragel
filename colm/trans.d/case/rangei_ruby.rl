#
# @LANG: ruby
# @GENERATED: true
#



%%{
	machine rangei;

	main := 
		'a' ../i 'z' .
		'A' ../i 'Z' .
		60 ../i 93 .
		94 ../i 125 .
		86 ../i 101 .
		60 ../i 125
		'';
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
	%% write init;
	%% write exec;
	if cs >= rangei_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"AaBbAa",
"Aa`bAa",
"AaB@Aa",
"AaBbMa",
"AaBbma",
]

inplen = 5

inp.each { |str| run_machine(str) }

