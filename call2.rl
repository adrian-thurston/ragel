/*
 * @LANG: c++
 */

#include <stdio.h>
#include <string.h>

int num = 0;

struct CallTest
{
	int cs, top, stack[32];

	// Initialize the machine. Invokes any init statement blocks. Returns 0
	// if the machine begins in a non-accepting state and 1 if the machine
	// begins in an accepting state.
	void init( );

	// Execute the machine on a block of data. Returns -1 if after processing
	// the data, the machine is in the error state and can never accept, 0 if
	// the machine is in a non-accepting state and 1 if the machine is in an
	// accepting state.
	void execute( const char *data, int len );

	// Indicate that there is no more data. Returns -1 if the machine finishes
	// in the error state and does not accept, 0 if the machine finishes
	// in any other non-accepting state and 1 if the machine finishes in an
	// accepting state.
	int finish( );
};

%%{
	machine CallTest;

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

void CallTest::init( )
{
	num = 0;
	%% write init;
}

void CallTest::execute( const char *data, int len )
{
	const char *p = data;
	const char *pe = data+len;

	%% write exec;
}

int CallTest::finish( )
{
	if ( this->cs == CallTest_error )
		return -1;
	if ( this->cs >= CallTest_first_final )
		return 1;
	return 0;
}

#define BUFSIZE 1024

void test( const char *buf )
{   
	CallTest test;

	test.init();
	test.execute( buf, strlen(buf) );
	if ( test.finish() > 0 )
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
