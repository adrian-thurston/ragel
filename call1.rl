/*
 * @LANG: c
 */

#include <stdio.h>
#include <string.h>

int num = 0;

struct test
{
	int cs, top, stack[32];
};

%%{ 
	machine test;
	access fsm->;

	action check_num {
		if ( num & 1 )
			fcall *fentry(odd);
		else
			fcall even;
	}

	# Test call and return functionality.
	even := 'even' any @{fhold; fret;};
	odd := 'odd' any @{fhold; fret;};
	num = [0-9]+ ${ num = num * 10 + (fc - '0'); };
	even_odd = num ' ' @check_num "\n";

	# Test calls in out actions.
	fail := !(any*);
	out_acts = 'OA ok\n' | 
		'OA error1\n' |
		'OA error2\n';

	main := even_odd | out_acts;
}%%

%% write data;

void test_init( struct test *fsm )
{
	num = 0;
	%% write init;
}

void test_execute( struct test *fsm, const char *data, int len )
{
	const char *p = data;
	const char *pe = data+len;

	%% write exec;
}

int test_finish( struct test *fsm )
{
	if ( fsm->cs == test_error )
		return -1;
	if ( fsm->cs >= test_first_final )
		return 1;
	return 0;
}

#define BUFSIZE 1024

void test( char *buf )
{   
	struct test test;
	test_init( &test );
	test_execute( &test, buf, strlen(buf) );
	if ( test_finish( &test ) > 0 )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

int main()
{
	test( "78 even\n" );
	test( "89 odd\n" );
	test( "1 even\n" );
	test( "0 odd\n" );
	test( "OA ok\n" );
	test( "OA error1\n" );
	test( "OA error2\n" );
	
	return 0;
}


#ifdef _____OUTPUT_____
ACCEPT
ACCEPT
FAIL
FAIL
ACCEPT
ACCEPT
ACCEPT
#endif
