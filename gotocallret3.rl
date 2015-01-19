/*
 * @LANG: indep
 * @NEEDS_EOF: yes
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

%%{
	machine gotocallret;

	# A reference to a state in an unused action caused a segfault in 5.8. */
	action unusedAction { fentry(garble_line); }

	action err_garbling_line { print_str "error: garbling line\n"; }
	action goto_main { fnext main; }
	action recovery_failed { print_str "error: failed to recover\n"; }

	# Error machine, consumes to end of 
	# line, then starts the main line over.
	garble_line := ( (any-'\n')*'\n') 
		>err_garbling_line
		@goto_main
		$/recovery_failed;

	action hold_and_return {fhold; fnret;}

	# Look for a string of alphas or of digits, 
	# on anything else, hold the character and return.
	alp_comm := alpha+ $!hold_and_return;
	dig_comm := digit+ $!hold_and_return;

	# Choose which to machine to call into based on the command.
	action comm_arg {
		if ( comm >= 'a' ) {
			fncall alp_comm;
		} else {
			fncall dig_comm;
		}
	}

	# Specifies command string. Note that the arg is left out.
	command = (
		[a-z0-9] @{comm = fc;} ' ' @comm_arg @{print_str "prints\n";} '\n'
	) @{print_str "correct command\n";};

	# Any number of commands. If there is an 
	# error anywhere, garble the line.
	main := command* $!{fhold;fnext garble_line;}; 
}%%

##### INPUT #####
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
##### OUTPUT #####
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
prints
error: garbling line
ACCEPT
error: garbling line
ACCEPT
prints
error: garbling line
ACCEPT
error: garbling line
ACCEPT
prints
correct command
ACCEPT
prints
correct command
ACCEPT
prints
correct command
ACCEPT
prints
correct command
ACCEPT
FAIL
prints
error: garbling line
error: failed to recover
FAIL
