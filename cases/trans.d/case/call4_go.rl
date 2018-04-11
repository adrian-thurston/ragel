/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var target  int ;
var top  int ;
var stack [32] int ;

%%{
	machine call4;

	unused := 'unused';

	one := 'one' @{fmt.Print( "one\n" );fret;
	
};

	two := 'two' @{fmt.Print( "two\n" );fret;
	
};

	main := (
			'1' @{target = fentry(one);
fcall *target; 
}
		|	'2' @{target = fentry(two);
fcall *target; 
}
		|	'\n'
	)*;
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
	%% write exec;
}
func finish() {
	if cs >= call4_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"1one2two1one\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
