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
		'b' => {
			print_str "pat2\n";
		};
        [ab] any*  => {
			print_str "pat3\n";
		};
	*|;
}%%

##### INPUT #####
"ab89"
##### OUTPUT #####
pat3
ACCEPT
