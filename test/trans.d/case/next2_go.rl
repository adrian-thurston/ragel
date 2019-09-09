/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var target  int ;
var last  int ;

%%{
	machine next2;

	unused := 'unused';

	one := 'one' @{fmt.Print( "one\n" );target = fentry(main);
fnext *target;
	
};

	two := 'two' @{fmt.Print( "two\n" );target = fentry(main);
fnext *target;
	
};

	three := 'three' @{fmt.Print( "three\n" );target = fentry(main);
fnext *target;
	
};

	main :=  (
		'1' @{target = fentry(one);
fnext *target;
			
last = 1;
} |

		'2' @{target = fentry(two);
fnext *target;
			
last = 2;
} |

		# This one is conditional based on the last.
		'3' @{if ( last == 2 ) {
	target = fentry(three);
fnext *target;
			

}
last = 3;
} 'x' |

		'\n'
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
	if cs >= next2_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"1one3x2two3three\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
