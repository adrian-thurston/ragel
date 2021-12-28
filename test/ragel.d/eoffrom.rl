/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 */

%%{
	machine eofact;

	action a1 { print_str "a1\n"; }
	action a2 { print_str "a2\n"; }

	main := "hello" ( "" %from a1 %from a2 );
}%%

##### INPUT #####
"hello"
"hello there"
##### OUTPUT #####
a1
a2
ACCEPT
a1
a2
FAIL
