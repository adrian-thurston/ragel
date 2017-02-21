/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var target  int ;

%%{
	machine next1;

	unused := 'unused';

	one := 'one' @{fmt.Print( "one\n" );target = fentry(main);
fnext *target;
	
};

	two := 'two' @{fmt.Print( "two\n" );target = fentry(main);
fnext *target;
	
};

	main := 
		'1' @{target = fentry(one);
fnext *target; 
}
	|	'2' @{target = fentry(two);
fnext *target; 
}
	|	'\n';
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
	if cs >= next1_first_final {
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
