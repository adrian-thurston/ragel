/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>

char comm ;
int top ;
int stack [32];

%%{
	machine GotoCallRet;

	# A reference to a state in an unused action caused a segfault in 5.8. */
	action unusedAction {fentry(garble_line);
}

	action err_garbling_line {printf( "%s", "error: garbling line\n" );
}
	action goto_main {fgoto main;}
	action recovery_failed {printf( "%s", "error: failed to recover\n" );
}

	# Error machine, consumes to end of 
	# line, then starts the main line over.
	garble_line := ( (any-'\n')*'\n') 
		>err_garbling_line
		@goto_main
		$/recovery_failed;

	action hold_and_return {fhold;fret;}

	# Look for a string of alphas or of digits, 
	# on anything else, hold the character and return.
	alp_comm := alpha+ $!hold_and_return;
	dig_comm := digit+ $!hold_and_return;

	# Choose which to machine to call into based on the command.
	action comm_arg {if ( comm >= 'a' )
{
	fcall alp_comm;
} 
else {
	fcall dig_comm;
}
}

	# Specifies command string. Note that the arg is left out.
	command = (
		[a-z0-9] @{comm = fc;
} ' ' @comm_arg '\n'
	) @{printf( "%s", "correct command\n" );
};

	# Any number of commands. If there is an 
	# error anywhere, garble the line.
	main := command* $!{fhold;fgoto garble_line;}; 
}%%



%% write data;
int cs;
int blen;
char buffer[1024];

void init()
{
	%% write init;
}

void exec( char *data, int len )
{
	char *p = data;
	char *pe = data + len;
	char *eof = pe;
	%% write exec;
}

void finish( )
{
	if ( cs >= GotoCallRet_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
"lkajsdf\n",
"2134\n",
"(\n",
"\n",
"*234234()0909 092 -234aslkf09`1 11\n",
"1\n",
"909\n",
"1 a\n",
"11 1\n",
"a 1\n",
"aa a\n",
"1 1\n",
"1 123456\n",
"a a\n",
"a abcdef\n",
"h",
"a aa1",
};

int inplen = 17;

int main( )
{
	int i;
	for ( i = 0; i < inplen; i++ ) {
		init();
		exec( inp[i], strlen(inp[i]) );
		finish();
	}
	return 0;
}

