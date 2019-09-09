/*
 * @LANG: indep
 *
 * Test in and out state actions.
 */

%%{
	machine state_act;

	action a1 { print_str "a1\n"; }
	action a2 { print_str "a2\n"; }
	action b1 { print_str "b1\n"; }
	action b2 { print_str "b2\n"; }
	action c1 { print_str "c1\n"; }
	action c2 { print_str "c2\n"; }
	action next_again {fnext again;}

	hi = 'hi';
	line = again: 
			hi 
				>to b1 
				>from b2 
			'\n' 
				>to c1 
				>from c2 
				@next_again;
		 
	main := line*
			>to a1 
			>from a2;
}%%

##### INPUT #####

"hi\nhi\n"

##### OUTPUT #####
a2
b2
c1
c2
b1
b2
c1
c2
b1
FAIL
