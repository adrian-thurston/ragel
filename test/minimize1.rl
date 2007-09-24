/*
 * @LANG: c
 */

#include <stdio.h>
#include <string.h>

struct min
{
	int cs;
};

%%{
	machine min;
	variable cs fsm->cs;

	action a_or_b { printf("a or b\n"); }

	main := (
		( 'a' . [ab]* @a_or_b ) |
		( 'b' . [ab]* @a_or_b ) 
	) . '\n';
}%%

%% write data;

void min_init( struct min *fsm )
{
	%% write init;
}

void min_execute( struct min *fsm, const char *_data, int _len )
{
	const char *p = _data;
	const char *pe = _data+_len;

	%% write exec;
}

int min_finish( struct min *fsm )
{
	if ( fsm->cs == min_error )
		return -1;
	if ( fsm->cs >= min_first_final )
		return 1;
	return 0;
}

struct min fsm;

void test( char *buf )
{
	int len = strlen( buf );
	min_init( &fsm );
	min_execute( &fsm, buf, len );
	if ( min_finish( &fsm ) > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}


int main()
{
	test( "aaaaaa\n" );
	test( "a\n" );
	test( "abc\n" );
	return 0;
}

#ifdef _____OUTPUT_____
a or b
a or b
a or b
a or b
a or b
ACCEPT
ACCEPT
a or b
FAIL
#endif
