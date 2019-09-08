/*
 * Demonstrate the use of goto, call and return. This machine expects either a
 * lower case char or a digit as a command then a space followed by the command
 * arg. If the command is a char, then the arg must be an a string of chars.
 * If the command is a digit, then the arg must be a string of digits. This
 * choice is determined by action code, rather than though transition
 * desitinations.
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace std;

struct GotoCallRet 
{
	char comm;
	int cs, top, stack[32];

	int init( );
	int execute( const char *data, int len, bool isEof );
	int finish( );
};

%%{
	machine GotoCallRet;

	# Error machine, consumes to end of 
	# line, then starts the main line over.
	garble_line := (
		(any-'\n')*'\n'
	) >{cout << "error: garbling line" << endl;} @{fgoto main;};

	# Look for a string of alphas or of digits, 
	# on anything else, hold the character and return.
	alp_comm := alpha+ $!{fhold;fret;};
	dig_comm := digit+ $!{fhold;fret;};

	# Choose which to machine to call into based on the command.
	action comm_arg {
		if ( comm >= 'a' )
			fcall alp_comm;
		else 
			fcall dig_comm;
	}

	# Specifies command string. Note that the arg is left out.
	command = (
		[a-z0-9] @{comm = fc;} ' ' @comm_arg '\n'
	) @{cout << "correct command" << endl;};

	# Any number of commands. If there is an 
	# error anywhere, garble the line.
	main := command* $!{fhold;fgoto garble_line;};
}%%

%% write data;

int GotoCallRet::init( )
{
	%% write init;
	return 1;
}

int GotoCallRet::execute( const char *data, int len, bool isEof )
{
	const char *p = data;
	const char *pe = data + len;
	const char *eof = isEof ? pe : 0;

	%% write exec;
	if ( cs == GotoCallRet_error )
		return -1;
	if ( cs >= GotoCallRet_first_final )
		return 1;
	return 0;
}

#define BUFSIZE 1024

int main()
{
	char buf[BUFSIZE];

	GotoCallRet gcr;
	gcr.init();
	while ( fgets( buf, sizeof(buf), stdin ) != 0 )
		gcr.execute( buf, strlen(buf), false );

	gcr.execute( 0, 0, true );
	if ( gcr.cs < GotoCallRet_first_final )
		cerr << "gotocallret: error: parsing input" << endl;
	return 0;
}
