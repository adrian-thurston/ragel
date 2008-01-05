/*
 * @LANG: c++
 */

#include <iostream>
using namespace std;

struct LangEl
{
	int key;
	const char *name;
};

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
	int execute(  LangEl *data, int len );

	// Indicate that there is no more data. Returns -1 if the machine finishes
	// in the error state and does not accept, 0 if the machine finishes
	// in any other non-accepting state and 1 if the machine finishes in an
	// accepting state.
	int finish( );

};

%%{
	machine Fsm;

	alphtype int;
	getkey fpc->key;
	variable eof eof_marker;

	action a1 {}
	action a2 {}
	action a3 {}

	main := ( 1 2* 3  ) 
			${cout << fpc->name << endl;} 
			%/{cout << "accept" << endl;};
}%%

%% write data;

int Fsm::init( )
{
	%% write init;
	return 0;
}

int Fsm::execute( LangEl *data, int len )
{
	LangEl *p = data;
	LangEl *pe = data + len;
	LangEl *eof_marker = pe;
	%% write exec;

	if ( cs == Fsm_error )
		return -1;
	if ( cs >= Fsm_first_final )
		return 1;
	return 0;
}

int Fsm::finish( )
{
	if ( cs == Fsm_error )
		return -1;
	if ( cs >= Fsm_first_final )
		return 1;
	return 0;
}

int main( )
{
	static Fsm fsm;
	static LangEl lel[] = { 
		{1, "one"}, 
		{2, "two-a"}, 
		{2, "two-b"}, 
		{2, "two-c"}, 
		{3, "three"}
	};

	fsm.init();
	fsm.execute( lel, 5 );
	fsm.finish();
	return 0;
}

#ifdef _____OUTPUT_____
one
two-a
two-b
two-c
three
accept
#endif
