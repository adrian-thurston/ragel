/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 */

int q_1;

%%{
	machine pattern_0001;

	action match { print_str "match\n"; }

	action ini_1 {
		print_str "ini = 0\n";
		q_1 = 0;
	}

	action inc_1 {
		print_str "inc to ";
		print_int q_1 + 1;
		print_str "\n";
		q_1 = q_1 + 1;
	}

	action min_1 { q_1 >= 5 }
	action max_1 { q_1 < 10 }

	main := 
		:condstar( 'a', ini_1, inc_1, min_1, max_1 ):
		%match;
}%%

##### INPUT #####
""
"a"
"aa"
"aaa"
"aaaa"
"aaaaa"
##### OUTPUT #####
ini = 0
FAIL
ini = 0
inc to 1
FAIL
ini = 0
inc to 1
inc to 2
FAIL
ini = 0
inc to 1
inc to 2
inc to 3
FAIL
ini = 0
inc to 1
inc to 2
inc to 3
inc to 4
FAIL
ini = 0
inc to 1
inc to 2
inc to 3
inc to 4
inc to 5
match
ACCEPT
