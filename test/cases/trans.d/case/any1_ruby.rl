#
# @LANG: ruby
# @GENERATED: true
#




%%{
	machine any1;
	main := any;
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
	if cs >= any1_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"",
"x",
"xx",
]

inplen = 3

inp.each { |str| run_machine(str) }

