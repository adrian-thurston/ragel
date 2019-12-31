/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 */
bool neg;
int val;
%%
%%{
	machine fbreak_eof;

	main := "hello\n" %{ fbreak; };
}%%
/* _____INPUT_____
"hello\n"
"there\n"
_____INPUT_____ */

/* _____OUTPUT_____
ACCEPT
FAIL
_____OUTPUT_____ */

