/*
 * @LANG: c
 */

#include <stdio.h>
#include <string.h>

struct range
{
	int cs;
};

%%{
	machine range;
	variable cs fsm->cs;

	main := ( 'a' .. 'c' | 'c' .. 'e' | 'm' .. 'n' | 'a' .. 'z' ) '\n';
}%%

%% write data;

void range_init( struct range *fsm )
{
	%% write init;
}

void range_execute( struct range *fsm, const char *_data, int _len )
{
	const char *p = _data;
	const char *pe = _data+_len;

	%% write exec;
}

int range_finish( struct range *fsm )
{
	if ( fsm->cs == range_error )
		return -1;
	if ( fsm->cs >= range_first_final )
		return 1;
	return 0;
}

struct range fsm;

void test( char *buf )
{
	int len = strlen( buf );
	range_init( &fsm );
	range_execute( &fsm, buf, len );
	if ( range_finish( &fsm ) > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}

int main()
{
	test( "a\n" );
	test( "z\n" );
	test( "g\n" );
	test( "no\n" );
	test( "1\n" );

	return 0;
}

#ifdef _____OUTPUT_____
ACCEPT
ACCEPT
ACCEPT
FAIL
FAIL
#endif
