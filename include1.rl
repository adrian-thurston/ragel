/*
 * @LANG: c
 * @IGNORE: yes
 *
 * Provides definitions for include tests.
 */

%%{
	machine include_test_1;

	action A {printf(" a1");}
	action B {printf(" b1");}

	action NonRef1 {printf(" nr1");}

	a1 = 'a' @A;
	b1 = 'b' @B;
}%%

%%{
	machine include_test_2;

	action NonRef2 {printf(" nr2");}

	a2 = 'a' @{printf(" a2");};
	b2 = 'b' @{printf(" b2");};
}%%

