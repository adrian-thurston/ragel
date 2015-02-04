/*
 * @LANG: indep
 */

int target;
int last;

%%{
	machine goto;

	unused := 'unused';

	one := 'one' @{
		print_str "one\n";
		target = fentry(main);
		fnext *target;
	};

	two := 'two' @{
		print_str "two\n";
		target = fentry(main);
		fnext *target;
	};

	three := 'three' @{
		print_str "three\n";
		target = fentry(main);
		fnext *target;
	};

	main :=  (
		'1' @{
			target = fentry(one);
			fnext *target;
			last = 1;
		} |

		'2' @{
			target = fentry(two);
			fnext *target;
			last = 2;
		} |

		# This one is conditional based on the last.
		'3' @{
			if ( last == 2 ) {
				target = fentry(three);
				fnext *target;
			}
				
			last = 3;
		} 'x' |

		'\n'
	)*;
}%%

##### INPUT #####
"1one3x2two3three\n"
##### OUTPUT #####
one
two
three
ACCEPT
