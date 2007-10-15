/*
 * @LANG: indep
 */

/*
 * Demonstrate the use of goto, call and return. This machine expects either a
 * lower case char or a digit as a command then a space followed by the command
 * arg. If the command is a char, then the arg must be an a string of chars.
 * If the command is a digit, then the arg must be a string of digits. This
 * choice is determined by action code, rather than though transition
 * desitinations.
 */

char comm;
int top;
int stack[32];
%%
%%{
	machine GotoCallRet;

	# A reference to a state in an unused action caused a segfault in 5.8. */
	action unusedAction { fentry(garble_line); }

	action err_garbling_line { prints "error: garbling line\n"; }
	action goto_main { fgoto main; }
	action recovery_failed { prints "error: failed to recover\n"; }

	# Error machine, consumes to end of 
	# line, then starts the main line over.
	garble_line := ( (any-'\n')*'\n') 
		>err_garbling_line
		@goto_main
		$/recovery_failed;

	action hold_and_return {fhold; fret;}

	# Look for a string of alphas or of digits, 
	# on anything else, hold the character and return.
	alp_comm := alpha+ $!hold_and_return;
	dig_comm := digit+ $!hold_and_return;

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
	) @{prints "correct command\n";};

	# Any number of commands. If there is an 
	# error anywhere, garble the line.
	main := command* $!{fhold;fgoto garble_line;}; 
}%%
/* _____INPUT_____
"lkajsdf\n"
"2134\n"
"(\n"
"\n"
"*234234()0909 092 -234aslkf09`1 11\n"
"1\n"
"909\n"
"1 a\n"
"11 1\n"
"a 1\n"
"aa a\n"
"1 1\n"
"1 123456\n"
"a a\n"
"a abcdef\n"
"h"
"a aa1"
_____INPUT_____ */
/* _____OUTPUT_____
error: garbling line
ACCEPT
error: garbling line
ACCEPT
error: garbling line
ACCEPT
error: garbling line
ACCEPT
error: garbling line
ACCEPT
error: garbling line
ACCEPT
error: garbling line
ACCEPT
error: garbling line
ACCEPT
error: garbling line
ACCEPT
error: garbling line
ACCEPT
error: garbling line
ACCEPT
correct command
ACCEPT
correct command
ACCEPT
correct command
ACCEPT
correct command
ACCEPT
error: failed to recover
FAIL
error: garbling line
error: failed to recover
FAIL
_____OUTPUT_____ */
