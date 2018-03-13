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
			print_str "on last     ";
			if ( p+1 == te ) {
				print_str "yes";
			}
			print_str "\n";
		};

		'b'+ => {
			print_str "on next     ";
			if ( p+1 == te ) {
				print_str "yes";
			}
			print_str "\n";
		};

		'c1' 'dxxx'? => {
			print_str "on lag      ";
			if ( p+1 == te ) {
				print_str "yes";
			}
			print_str "\n";
		};

		'd1' => {
			print_str "lm switch1  ";
			if ( p+1 == te ) {
				print_str "yes";
			}
			print_str "\n";
		};
		'd2' => {
			print_str "lm switch2  ";
			if ( p+1 == te ) {
				print_str "yes";
			}
			print_str "\n";
		};

		[d0-9]+ '.';

		'\n';
	*|;
}%%

##### INPUT #####
"abbc1d1d2\n"
##### OUTPUT #####
on last     yes
on next     yes
on lag      yes
lm switch1  yes
lm switch2  yes
ACCEPT
