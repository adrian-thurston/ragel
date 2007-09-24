/*
 * @LANG: c
 */

/*
 * Test of a transition going to the error state.
 */

#include <stdio.h>
#define BUFSIZE 2048

struct errintrans
{
	int cs;
};

%%{
	machine errintrans;
	variable cs fsm->cs;

	char = any - (digit | '\n');
	line = char* "\n";
	main := line+;
}%%

%% write data;

void errintrans_init( struct errintrans *fsm )
{
	%% write init;
}

void errintrans_execute( struct errintrans *fsm, const char *_data, int _len )
{
	const char *p = _data;
	const char *pe = _data+_len;

	%% write exec;
}

int errintrans_finish( struct errintrans *fsm )
{
	if ( fsm->cs == errintrans_error )
		return -1;
	if ( fsm->cs >= errintrans_first_final )
		return 1;
	return 0;
}


struct errintrans fsm;
#include <string.h>

void test( char *buf )
{
	int len = strlen( buf );
	errintrans_init( &fsm );
	errintrans_execute( &fsm, buf, len );
	if ( errintrans_finish( &fsm ) > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}


int main()
{
	test(
		"good, does not have numbers\n"
	);

	test(
		"bad, has numbers 666\n"
	);

	return 0;
}

#ifdef _____OUTPUT_____
ACCEPT
FAIL
#endif
