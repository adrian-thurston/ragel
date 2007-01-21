/*
 * @LANG: indep
 * @ALLOW_GENFLAGS: -T0 -T1 -F0 -F1 -G0 -G1 -G2 -P
 */
%%
%%{
	machine eofact;

	action a1 { prints "a1\n"; }
	action a2 { prints "a2\n"; }
	action a3 { prints "a3\n"; }
	action a4 { prints "a4\n"; }


	main := (
		'hello' @eof a1 %eof a2 '\n'? |
		'there' @eof a3 %eof a4
	);

}%%
/* _____INPUT_____
""
"h"
"hell"
"hello"
"hello\n"
"t"
"ther"
"there"
"friend"
_____INPUT_____ */
/* _____OUTPUT_____
a1
a3
FAIL
a1
FAIL
a1
FAIL
a2
ACCEPT
ACCEPT
a3
FAIL
a3
FAIL
a4
ACCEPT
FAIL
_____OUTPUT_____ */
