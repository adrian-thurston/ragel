/*
 * @LANG: c
 */

#include <stdio.h>
#include <string.h>

%%{
	machine include_test_4;

	action NonRef3 {printf(" nr3");}

	a3 = 'a'@{printf(" a3");};
	b3 = 'b'@{printf(" b3");};

}%%

%%{
	machine include_test_1;

	include "include1.rl";

	include include_test_2 "include1.rl";

	include include_test_4;

	main := 
		a1 b1 @NonRef1 
		a2 b2 @NonRef2 
		a3 b3 @NonRef3
		0 @{fbreak;};
}%%

%% write data;

void test( char *p )
{
	int cs;
	%% write init;
	%% write exec noend;
	printf("\n");
}

int main()
{
	test( "ababab" );
	return 0;
}

#ifdef _____OUTPUT_____
 a1 b1 nr1 a2 b2 nr2 a3 b3 nr3
#endif
