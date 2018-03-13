/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var neg  int ;
var value  int ;

%%{
	machine state_chart;

	action begin {neg = 0;
value = 0;
}

	action see_neg {neg = 1;
}

	action add_digit {value = value * 10 + ( int ) ( fc - 48 )
;
}

	action finish {if ( neg != 0 ) {
	value = -1 * value;

}
}

	atoi = (
		start: (
			'-' @see_neg ->om_num | 
			'+' ->om_num |
			[0-9] @add_digit ->more_nums
		),

		# One or more nums.
		om_num: (
			[0-9] @add_digit ->more_nums
		),

		# Zero ore more nums.
		more_nums: (
			[0-9] @add_digit ->more_nums |
			'' -> final
		)
	) >begin %finish;

	action oneof {fmt.Print( value );fmt.Print( "\n" );}
	main := ( atoi '\n' @oneof )*;
}%%


var cs int;
var blen int;
var buffer [1024] byte;

%% write data;

func prepare() {
value = 0;
neg = 0;
	%% write init;
}

func exec(data string) {
	var p int = 0
	var pe int = len(data)
	%% write exec;
}
func finish() {
	if cs >= state_chart_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"1\n",
"12\n",
"222222\n",
"+2123\n",
"213 3213\n",
"-12321\n",
"--123\n",
"-99\n",
" -3000\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
