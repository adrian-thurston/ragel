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
		'b' => {
			prints "pat2\n";
		};
        [ab] any*  => {
			prints "pat3\n";
		};
	*|;
}%%
/* _____INPUT_____
"ab89"
_____INPUT_____ */
/* _____OUTPUT_____
pat3
ACCEPT
_____OUTPUT_____ */
