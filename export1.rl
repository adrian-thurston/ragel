/*
 * @LANG: c
 */

#include <stdio.h>
#include <string.h>

%%{
	machine test;

	export c1 = 'c';
	export c2 = 'z';
	export c3 = 't';

	commands := (
		c1 . digit* '\n' @{ printf( "c1\n" );} |
		c2 . alpha* '\n' @{ printf( "c2\n" );}|
		c3 . '.'* '\n' @{ printf( "c3\n" );}
	)*;
		
	some_other := any*;
}%%

%% write exports;
%% write data;

int test( const char *data, int len )
{
	int cs = test_en_commands;
	const char *p = data, *pe = data + len;

	%% write init nocs;
	%% write exec;

	if ( cs >= test_first_final )
		printf("ACCEPT\n");
	else
		printf("ERROR\n");
	return 0;
}

char data[] = { 
	test_ex_c1, '1', '2', '\n', 
	test_ex_c2, 'a', 'b', '\n', 
	test_ex_c3, '.', '.', '\n', 0 
};

int main()
{
	test( data, strlen( data ) );
	return 0;
}

#ifdef _____OUTPUT_____
c1
c2
c3
ACCEPT
#endif
