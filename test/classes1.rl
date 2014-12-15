/*
 * @LANG: c
 */

#include <stdio.h>
#include <string.h>

struct classes
{
	int cs;
};

%%{
	machine classes;
	variable cs fsm->cs;

	action a_az { printf("a-z\n"); }
	action a_09 { printf("0-9\n"); }
	action a_full { printf("full\n"); }

	main := (
		( [a-z] @a_az )
		( [0-9] @a_09 )
		( 0 .. 127 @a_full )
	)* . '\n';
}%%

%% write data;

void classes_init( struct classes *fsm )
{
	%% write init;
}

void classes_execute( struct classes *fsm, const char *_data, int _len )
{
	const char *p = _data;
	const char *pe = _data+_len;

	%% write exec;
}

int classes_finish( struct classes *fsm )
{
	if ( fsm->cs == classes_error )
		return -1;
	if ( fsm->cs >= classes_first_final )
		return 1;
	return 0;
}

struct classes fsm;

void test( char *buf )
{
	int len = strlen( buf );
	classes_init( &fsm );
	classes_execute( &fsm, buf, len );
}


int main()
{
	test( "a0X\n" );
	return 0;
}

#ifdef _____OUTPUT_____
a-z
0-9
full
#endif
