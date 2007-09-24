#
# @LANG: ruby
#

%%{
    machine atoi3;
    action begin {
        neg = false;
        val = 0;
    }
    action see_neg {
        neg = true;
    }
    action add_digit {
		val = val * 10 + (fc - "0"[0]);
    }
    action finish {
		val = -1 * val if neg
    }
    action print {
        puts val;
    }
    atoi = (('-' @ see_neg | '+') ? (digit @ add_digit) +) > begin % finish;
    main := atoi '\n' @ print;
}%%

%% write data;

def run_machine( data )
	p = 0;
	pe = data.length
	cs = 0
	val = 0;
	neg = false;

	%% write init;
	%% write exec;
	if  cs >= atoi3_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
		"1\n",
		"12\n",
		"222222\n",
		"+2123\n",
		"213 3213\n",
		"-12321\n",
		"--123\n",
		"-99\n",
		" -3000\n",
]

inp.each { |str| run_machine(str) }

=begin _____OUTPUT_____
1
ACCEPT
12
ACCEPT
222222
ACCEPT
2123
ACCEPT
FAIL
-12321
ACCEPT
FAIL
-99
ACCEPT
FAIL
=end _____OUTPUT_____
