/*
 * @LANG: indep
 *
 * Test in and out state actions.
 */
%%
%%{
	machine state_act;

	action a1 { prints "a1\n"; }
	action a2 { prints "a2\n"; }
	action b1 { prints "b1\n"; }
	action b2 { prints "b2\n"; }
	action c1 { prints "c1\n"; }
	action c2 { prints "c2\n"; }
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

/* _____INPUT_____
"hi\nhi\n"
_____INPUT_____ */

/* _____OUTPUT_____
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
_____OUTPUT_____ */
