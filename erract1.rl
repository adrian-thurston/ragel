/*
 * @LANG: c++
 */

/*
 * Test error actions.
 */

#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

struct ErrAct 
{
	int cs;

	// Initialize the machine. Invokes any init statement blocks. Returns 0
	// if the machine begins in a non-accepting state and 1 if the machine
	// begins in an accepting state.
	int init( );

	// Execute the machine on a block of data. Returns -1 if after processing
	// the data, the machine is in the error state and can never accept, 0 if
	// the machine is in a non-accepting state and 1 if the machine is in an
	// accepting state.
	int execute( const char *data, int len );

	// Indicate that there is no more data. Returns -1 if the machine finishes
	// in the error state and does not accept, 0 if the machine finishes
	// in any other non-accepting state and 1 if the machine finishes in an
	// accepting state.
	int finish( );
};

%%{
	machine ErrAct;

	action expect_digit_plus_minus { printf(" DIGIT PLUS MINUS\n"); } 
	action expect_digit { printf(" DIGIT\n"); } 
	action expect_digit_decimal { printf(" DIGIT DECIMAL\n"); }

	float = (
		( 
			[\-+] >err expect_digit_plus_minus %err expect_digit |
			""
		)
		( [0-9] [0-9]* $err expect_digit_decimal )
		( '.' [0-9]+ $err expect_digit )? 
	);

	main := float '\n';
}%%

%% write data;

int ErrAct::init( )
{
	%% write init;
	return 0;
}

int ErrAct::execute( const char *_data, int _len )
{
	const char *p = _data;
	const char *pe = _data+_len;
	const char *eof = pe;
	%% write exec;

	if ( cs == ErrAct_error )
		return -1;
	if ( cs >= ErrAct_first_final )
		return 1;
	return 0;
}

int ErrAct::finish( )
{
	if ( cs == ErrAct_error )
		return -1;
	if ( cs >= ErrAct_first_final )
		return 1;
	return 0;
}

#define BUFSIZE 1024

void test( const char *buf )
{
	ErrAct errAct;
	errAct.init();
	errAct.execute( buf, strlen(buf) );
	if ( errAct.finish() > 0 )
		cout << "ACCEPT" << endl;
	else
		cout << "FAIL" << endl;
}

int main()
{
	test( "1\n" );
	test( "+1\n" );
	test( "-1\n" );
	test( "1.1\n" );
	test( "+1.1\n" );
	test( "-1.1\n" );
	test( "a\n" );
	test( "-\n" );
	test( "+\n" );
	test( "-a\n" );
	test( "+b\n" );
	test( "1.\n" );
	test( "1d\n" );
	test( "1.d\n" );
	test( "1.1d\n" );
	return 0;
}

#ifdef _____OUTPUT_____
ACCEPT
ACCEPT
ACCEPT
ACCEPT
ACCEPT
ACCEPT
 DIGIT PLUS MINUS
FAIL
 DIGIT
FAIL
 DIGIT
FAIL
 DIGIT
FAIL
 DIGIT
FAIL
 DIGIT
FAIL
 DIGIT DECIMAL
FAIL
 DIGIT
FAIL
 DIGIT
FAIL
#endif
