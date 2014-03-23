/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 */

char comm;
int top;
int stack[32];
ptr ts;
ptr te;
int act;
int val;

%%{
	machine patact;

	other := |* 
		[a-z]+ => { print_str "word\n"; };
		[0-9]+ => { print_str "num\n"; };
		[\n ] => { print_str "space\n"; };
	*|;

	exec_test := |* 
		[a-z]+ => { print_str "word (w/lbh)\n"; fexec te-1; fgoto other; };
		[a-z]+ ' foil' => { print_str "word (c/lbh)\n"; };
		[\n ] => { print_str "space\n"; };
		'22' => { print_str "num (w/switch)\n"; };
		[0-9]+ => { print_str "num (w/switch)\n"; fexec te-1; fgoto other;};
		[0-9]+ ' foil' => {print_str "num (c/switch)\n"; };
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => { print_str "in semi\n"; fgoto main; };
	*|;

	main := |* 
		[a-z]+ => { print_str "word (w/lbh)\n"; fhold; fgoto other; };
		[a-z]+ ' foil' => { print_str "word (c/lbh)\n"; };
		[\n ] => { print_str "space\n"; };
		'22' => { print_str "num (w/switch)\n"; };
		[0-9]+ => { print_str "num (w/switch)\n"; fhold; fgoto other;};
		[0-9]+ ' foil' => {print_str "num (c/switch)\n"; };
		';' => { print_str "going to semi\n"; fhold; fgoto semi;};
		'!' => { print_str "immdiate\n"; fgoto exec_test; };
	*|;
}%%

#ifdef _____INPUT_____
"abcd foix\n"
"abcd\nanother\n"
"123 foix\n"
"!abcd foix\n"
"!abcd\nanother\n"
"!123 foix\n"
";"
#endif

#ifdef _____OUTPUT_____
word (w/lbh)
word
space
word
space
ACCEPT
word (w/lbh)
word
space
word
space
ACCEPT
num (w/switch)
num
space
word
space
ACCEPT
immdiate
word (w/lbh)
word
space
word
space
ACCEPT
immdiate
word (w/lbh)
word
space
word
space
ACCEPT
immdiate
num (w/switch)
num
space
word
space
ACCEPT
going to semi
in semi
ACCEPT
#endif
