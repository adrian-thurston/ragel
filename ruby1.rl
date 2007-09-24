#
# @LANG: ruby
#
# Test the host language scanning for ruby.
#

# %%{
a = 1
b = /%%\{\}/;

%%{
    machine ruby1;

    main := lower+ digit+ '\n' @{

		# }
		c = 1
		d = /\}/
		puts "NL"
	};
}%%

# %%{
e = 1
f = /%%\{\}/;

%% write data;

# %%{
g = 1
h = /%%\{\}/;

def run_machine( data )
	p = 0;
	pe = data.length
	cs = 0

	%% write init;
	%% write exec;
	if  cs >= ruby1_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
		"abc1231\n",
]

inp.each { |str| run_machine(str) }

=begin _____OUTPUT_____
NL
ACCEPT
=end _____OUTPUT_____
