/*
 * @LANG: c++
 */

/**
 * Test a high character to make sure signedness 
 * isn't messing us up.
 */

#include <stdio.h>
#include <string.h>

struct Fsm
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
	int execute( const unsigned char *data, int len );

	// Indicate that there is no more data. Returns -1 if the machine finishes
	// in the error state and does not accept, 0 if the machine finishes
	// in any other non-accepting state and 1 if the machine finishes in an
	// accepting state.
	int finish( );
};

%%{
	machine Fsm;

	alphtype unsigned char;

	# Indicate we got the high character.
	action gothigh {
		printf("yes\n");
	}

	main := 0xe8 @gothigh '\n';
}%%

%% write data;

int Fsm::init( )
{
	%% write init;
	return 0;
}

int Fsm::execute( const unsigned char *_data, int _len )
{
	const unsigned char *p = _data;
	const unsigned char *pe = _data+_len;
	%% write exec;
	if ( cs == Fsm_error )
		return -1;
	if ( cs >= Fsm_first_final )
		return 1;
	return 0;
}

int Fsm::finish()
{
	if ( cs == Fsm_error )
		return -1;
	if ( cs >= Fsm_first_final )
		return 1;
	return 0;
}

Fsm fsm;

void test( unsigned char *buf, int len )
{
	fsm.init();
	fsm.execute( buf, len );
	if ( fsm.finish() > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}

unsigned char data1[] = { 0xe8, 10 };
unsigned char data2[] = { 0xf8, 10 };

int main()
{
	test( data1, 2 );
	test( data2, 2 );
	return 0;
}

#ifdef _____OUTPUT_____
yes
ACCEPT
FAIL
#endif
