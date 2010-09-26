// -*-go-*-
//
// Convert a string to an integer.
//
// To compile:
//
//   ragel -Z -G2 -o atoi.go atoi.rl
//   6g -o atoi.6 atoi.go
//   6l -o atoi atoi.6
//   ./atoi
//
// To show a diagram of your state machine:
//
//   ragel -V -G2 -p -o atoi.dot atoi.rl
//   dot -Tpng -o atoi.png atoi.dot
//   chrome atoi.png
//

package main

import (
	"os"
	"fmt"
)

%%{
	machine atoi;
	write data;
}%%

func atoi(data string) (val int) {
	cs, p, pe := 0, 0, len(data)
	neg := false

	%%{
		action see_neg   { neg = true }
		action add_digit { val = val * 10 + (int(fc) - '0') }

		main :=
			( '-'@see_neg | '+' )? ( digit @add_digit )+
			'\n'?
			;

		write init;
		write exec;
	}%%

	if neg {
		val = -1 * val;
	}

	if cs < atoi_first_final {
		fmt.Println("atoi: there was an error:", cs, "<", atoi_first_final)
		fmt.Println(data)
		for i := 0; i < p; i++ {
			fmt.Print(" ")
		}
		fmt.Println("^")
	}

	return val
}

//////////////////////////////////////////////////////////////////////

type atoiTest struct {
	s string
	v int
}

var atoiTests = []atoiTest{
	atoiTest{"7", 7},
	atoiTest{"666", 666},
	atoiTest{"-666", -666},
	atoiTest{"+666", 666},
	atoiTest{"1234567890", 1234567890},
	atoiTest{"+1234567890\n", 1234567890},
	// atoiTest{"+ 1234567890", 1234567890}, // i will fail
}

func main() {
	res := 0
	for _, test := range atoiTests {
		res := atoi(test.s)
		if res != test.v {
			fmt.Fprintf(os.Stderr, "FAIL atoi(%#v) != %#v\n", test.s, test.v)
			res = 1
		}
	}
	os.Exit(res)
}
