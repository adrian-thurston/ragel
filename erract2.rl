/*
 * @LANG: indep
 *
 * Test error actions.
 */
%%
%%{
	machine ErrAct;

	action err_start { prints "err_start\n"; }
	action err_all { prints "err_all\n"; }
	action err_middle { prints "err_middle\n"; }
	action err_out { prints "err_out\n"; }

	action eof_start { prints "eof_start\n"; }
	action eof_all { prints "eof_all\n"; }
	action eof_middle { prints "eof_middle\n"; }
	action eof_out { prints "eof_out\n"; }

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
		) '\n';
}%%

/* _____INPUT_____
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
_____INPUT_____ */

/* _____OUTPUT_____
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
_____OUTPUT_____ */
