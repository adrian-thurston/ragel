/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 *
 * Test error actions.
 */

%%{
	machine erract;

	action err_start { print_str "err_start\n"; }
	action err_all { print_str "err_all\n"; }
	action err_middle { print_str "err_middle\n"; }
	action err_out { print_str "err_out\n"; }

	action eof_start { print_str "eof_start\n"; }
	action eof_all { print_str "eof_all\n"; }
	action eof_middle { print_str "eof_middle\n"; }
	action eof_out { print_str "eof_out\n"; }

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
		) '\n';
}%%

##### INPUT #####
""
"h"
"x"
"he"
"hx"
"hel"
"hex"
"hell"
"helx"
"hello"
"hellx"
"hello\n"
"hellox"
##### OUTPUT #####
err_start
eof_start
err_all
eof_all
FAIL
err_all
err_middle
eof_all
eof_middle
FAIL
err_start
err_all
FAIL
err_all
err_middle
eof_all
eof_middle
FAIL
err_all
err_middle
FAIL
err_all
err_middle
eof_all
eof_middle
FAIL
err_all
err_middle
FAIL
err_all
err_middle
eof_all
eof_middle
FAIL
err_all
err_middle
FAIL
err_all
err_out
eof_all
eof_out
FAIL
err_all
err_middle
FAIL
ACCEPT
err_all
err_out
FAIL
