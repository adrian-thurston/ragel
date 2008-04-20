/*
 * @LANG: indep
 */
ptr ts;
ptr te;
int act;
int token;
%%
%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
        'a' => {
			prints "pat1\n";
		};

        [ab]+ . 'c' => {
			prints "pat2\n";
		};

        any => {
			prints "any\n";
		};
	*|;
}%%
/* _____INPUT_____
"a"
_____INPUT_____ */
/* _____OUTPUT_____
pat1
ACCEPT
_____OUTPUT_____ */
