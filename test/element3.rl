/*
 * @LANG: obj-c
 */

#include <stdio.h>
#include <objc/Object.h>

struct LangEl
{
	int key;
	char *name;
};

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
- (int) executeWithData:( struct LangEl *)data len:(int)len;

// Indicate that there is no more data. Returns -1 if the machine finishes
// in the error state and does not accept, 0 if the machine finishes
// in any other non-accepting state and 1 if the machine finishes in an
// accepting state.
- (int) finish;

@end;


@implementation Fsm

%%{
	machine Fsm;

	alphtype int;
	getkey fpc->key;

	action a1 {}
	action a2 {}
	action a3 {}

	main := ( 1 2* 3  ) 
			${printf("%s\n", fpc->name);} 
			%/{printf("accept\n");};
}%%

%% write data;

- (int) initFsm;
{
	%% write init;
	return 0;
}

- (int) executeWithData:( struct LangEl *)_data len:(int)_len;
{
	struct LangEl *p = _data;
	struct LangEl *pe = _data + _len;
	struct LangEl *eof = pe;
	%% write exec;

	if ( self->cs == Fsm_error ) 
		return -1;
	return ( self->cs >= Fsm_first_final ) ? 1 : 0;
}

- (int) finish;
{
	if ( self->cs == Fsm_error ) 
		return -1;
	return ( self->cs >= Fsm_first_final ) ? 1 : 0;
}


@end

int main()
{
	static Fsm *fsm;
	static struct LangEl lel[] = { 
		{1, "one"}, 
		{2, "two-a"}, 
		{2, "two-b"}, 
		{2, "two-c"}, 
		{3, "three"}
	};
	
	fsm = [[Fsm alloc] init];
	[fsm initFsm];
	[fsm executeWithData:lel len:5];
	[fsm finish];

	return 0;
}

@interface Fsm2 : Object
{
	// The current state may be read and written to from outside of the
	// machine.  From within action code, curs is -1 and writing to it has no
	// effect.
	@public
	int cs;

	@protected

}

// Execute the machine on a block of data. Returns -1 if after processing
// the data, the machine is in the error state and can never accept, 0 if
// the machine is in a non-accepting state and 1 if the machine is in an
// accepting state.
- (int)
executeWithElements:(int) elements
length:(unsigned)length;

@end

@implementation Fsm2
- (int)
executeWithElements:(int)elements
length:(unsigned)length;
{
	return 0;
}
@end

#ifdef _____OUTPUT_____
one
two-a
two-b
two-c
three
accept
#endif
