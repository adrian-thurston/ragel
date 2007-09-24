/*
 * @LANG: obj-c
 */

/**
 * Test a high character to make sure signedness 
 * isn't messing us up.
 */

#include <stdio.h>
#include <objc/Object.h>

@interface Fsm : Object
{
@public
	int cs;
};

// Initialize the machine. Invokes any init statement blocks. Returns 0
// if the machine begins in a non-accepting state and 1 if the machine
// begins in an accepting state.
- (int) initFsm;

// Execute the machine on a block of data. Returns -1 if after processing
// the data, the machine is in the error state and can never accept, 0 if
// the machine is in a non-accepting state and 1 if the machine is in an
// accepting state.
- (void) executeWithData:(const unsigned char *)data len:(int)len;

// Indicate that there is no more data. Returns -1 if the machine finishes
// in the error state and does not accept, 0 if the machine finishes
// in any other non-accepting state and 1 if the machine finishes in an
// accepting state.
- (int) finish;

@end

@implementation Fsm

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

- (int) initFsm;
{
	%% write init;
	return 1;
}

- (void) executeWithData:(const unsigned char *)_data len:(int)_len;
{
	const unsigned char *p = _data;
	const unsigned char *pe = _data + _len;
	%% write exec;
}

- (int) finish;
{
	if ( cs == Fsm_error )
		return -1;
	else if ( cs >= Fsm_first_final )
		return 1;
	return 0;
}


@end


#define BUFSIZE 2048

Fsm *fsm;
unsigned char buf[BUFSIZE];

void test( unsigned char *buf, int len )
{
	fsm = [[Fsm alloc] init];
	[fsm initFsm];
	[fsm executeWithData:buf len:len];
	if ( [fsm finish] > 0 )
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
