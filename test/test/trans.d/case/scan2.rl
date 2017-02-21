/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 */
ptr ts;
ptr te;
int act;
int token;

%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
        'a' => {
			print_str "pat1\n";
		};

        [ab]+ . 'c' => {
			print_str "pat2\n";
		};

        any => {
			print_str "any\n";
		};
	*|;
}%%

##### INPUT #####
"a"
##### OUTPUT #####
pat1
ACCEPT
