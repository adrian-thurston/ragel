/*
 * @LANG: obj-c
 */

#include <stdio.h>
#include <objc/Object.h>

#define IDENT_BUFLEN 256

@interface ErrAct : Object
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
- (void) executeWithData:(const char *)data len:(int)len;

// Indicate that there is no more data. Returns -1 if the machine finishes
// in the error state and does not accept, 0 if the machine finishes
// in any other non-accepting state and 1 if the machine finishes in an
// accepting state.
- (int) finish;

@end

@implementation ErrAct

%%{
	machine ErrAct;

	# The data that is to go into the fsm structure.
	action hello_fails { printf("hello fails\n");}

	newline = ( any | '\n' @{printf("newline\n");} )*;
	hello = 'hello\n'* $^hello_fails @/hello_fails;
	main := newline | hello;
}%%

%% write data;

- (int) initFsm;
{
	%% write init;
	return 1;
}

- (void) executeWithData:(const char *)_data len:(int)_len;
{
	const char *p = _data;
	const char *pe = _data + _len;
	const char *eof = pe;
	%% write exec;
}

- (int) finish;
{
	if ( cs == ErrAct_error )
		return -1;
	else if ( cs >= ErrAct_first_final )
		return 1;
	return 0;
}

@end

#include <stdio.h>
#include <string.h>
#define BUFSIZE 2048

ErrAct *fsm;
char buf[BUFSIZE];

void test( char *buf )
{
	int len = strlen(buf);
	fsm = [[ErrAct alloc] init];
	
	[fsm initFsm];
	[fsm executeWithData:buf len:len];
	if ( [fsm finish] > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}


int main()
{
	test(
		"hello\n"
		"hello\n"
		"hello\n"
	);

	test(
		"hello\n"
		"hello\n"
		"hello there\n"
	);

	test(
		"hello\n"
		"hello\n"
		"he"	);

	test( "" );

	return 0;
}

#ifdef _____OUTPUT_____
newline
newline
newline
ACCEPT
newline
newline
hello fails
newline
ACCEPT
newline
newline
hello fails
ACCEPT
ACCEPT
#endif
