/*
 * @LANG: indep
 */

char comm;
int top;
int stack[32];
ptr ts;
ptr te;
int act;
int val;
%%
%%{
	machine patact;

	other := |* 
		[a-z]+ => { prints "word\n"; };
		[0-9]+ => { prints "num\n"; };
		[\n ] => { prints "space\n"; };
	*|;

	exec_test := |* 
		[a-z]+ => { prints "word (w/lbh)\n"; fexec te-1; fgoto other; };
		[a-z]+ ' foil' => { prints "word (c/lbh)\n"; };
		[\n ] => { prints "space\n"; };
		'22' => { prints "num (w/switch)\n"; };
		[0-9]+ => { prints "num (w/switch)\n"; fexec te-1; fgoto other;};
		[0-9]+ ' foil' => {prints "num (c/switch)\n"; };
		'!';# => { prints "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => { prints "in semi\n"; fgoto main; };
	*|;

	main := |* 
		[a-z]+ => { prints "word (w/lbh)\n"; fhold; fgoto other; };
		[a-z]+ ' foil' => { prints "word (c/lbh)\n"; };
		[\n ] => { prints "space\n"; };
		'22' => { prints "num (w/switch)\n"; };
		[0-9]+ => { prints "num (w/switch)\n"; fhold; fgoto other;};
		[0-9]+ ' foil' => {prints "num (c/switch)\n"; };
		';' => { prints "going to semi\n"; fhold; fgoto semi;};
		'!' => { prints "immdiate\n"; fgoto exec_test; };
	*|;
}%%
/* _____INPUT_____
"abcd foix\n"
"abcd\nanother\n"
"123 foix\n"
"!abcd foix\n"
"!abcd\nanother\n"
"!123 foix\n"
";"
_____INPUT_____ */
/* _____OUTPUT_____
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
_____OUTPUT_____ */

