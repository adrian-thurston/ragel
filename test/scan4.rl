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

	  any;
	*|;
}%%

#ifdef _____INPUT_____
"ba a"
#endif

#ifdef _____OUTPUT_____
pat1
pat1
ACCEPT
#endif
