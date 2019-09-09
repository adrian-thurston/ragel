/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 */

ptr ts;
ptr te;
%%{
	machine lmnfa1;

	main := :nfa |*
		"hello" %when {false} => { print_str "hello --fail\n"; };
		"hello"               => { print_str "hello\n"; };
		[a-z]+ %when {false}  => { print_str "other --fail\n"; };
		[a-z]+                => { print_str "other\n"; };
		' '                   => { print_str "<space>\n"; };
	*|;
}%%

##### INPUT #####
"hello hellos hello"
##### OUTPUT #####
hello
<space>
other
<space>
hello
ACCEPT
