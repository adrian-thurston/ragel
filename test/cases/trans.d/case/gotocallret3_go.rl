/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var comm  byte ;
var top  int ;
var stack [32] int ;

%%{
	machine gotocallret;

	# A reference to a state in an unused action caused a segfault in 5.8. */
	action unusedAction {fentry(garble_line);
}

	action err_garbling_line {fmt.Print( "error: garbling line\n" );}
	action goto_main {fnext main; 
}
	action recovery_failed {fmt.Print( "error: failed to recover\n" );}

	# Error machine, consumes to end of 
	# line, then starts the main line over.
	garble_line := ( (any-'\n')*'\n') 
		>err_garbling_line
		@goto_main
		$/recovery_failed;

	action hold_and_return {fhold; 
fnret;
}

	# Look for a string of alphas or of digits, 
	# on anything else, hold the character and return.
	alp_comm := alpha+ $!hold_and_return;
	dig_comm := digit+ $!hold_and_return;

	# Choose which to machine to call into based on the command.
	action comm_arg {if ( comm >= 97 ) {
	fncall alp_comm;
		

} else {
	fncall dig_comm;
		

}
}

	# Specifies command string. Note that the arg is left out.
	command = (
		[a-z0-9] @{comm = fc;
} ' ' @comm_arg @{fmt.Print( "prints\n" );} '\n'
	) @{fmt.Print( "correct command\n" );};

	# Any number of commands. If there is an 
	# error anywhere, garble the line.
	main := command* $!{fhold;
fnext garble_line;
}; 
}%%


var cs int;
var blen int;
var buffer [1024] byte;

%% write data;

func prepare() {
	%% write init;
}

func exec(data string) {
	var p int = 0
	var pe int = len(data)
	var eof int = pe
	%% write exec;
}
func finish() {
	if cs >= gotocallret_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
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

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
