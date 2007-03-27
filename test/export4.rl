/*
 * @LANG: d
 */

import std.c.stdio;
import std.string;

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

int test( char data[] )
{
	int cs = test_en_commands;
	char *p = data.ptr, pe = data.ptr + data.length;

	%% write init nocs;
	%% write exec;

	if ( cs >= test_first_final )
		printf("ACCEPT\n");
	else
		printf("ERROR\n");
	return 0;
}

char data[] = [ 
	test_ex_c1, '1', '2', '\n', 
	test_ex_c2, 'a', 'b', '\n', 
	test_ex_c3, '.', '.', '\n'
];

int main()
{
	test( data );
	return 0;
}

/+ _____OUTPUT_____
c1
c2
c3
ACCEPT
++++++++++++++++++/
