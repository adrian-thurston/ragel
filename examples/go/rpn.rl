// -*-go-*-
//
// Reverse Polish Notation Calculator
// Copyright (c) 2010 J.A. Roberts Tunney
// MIT License
//
// To compile:
//
//   ragel -Z -G2 -o rpn.go rpn.rl
//   6g -o rpn.6 rpn.go
//   6l -o rpn rpn.6
//   ./rpn
//
// To show a diagram of your state machine:
//
//   ragel -V -G2 -p -o rpn.dot rpn.rl
//   dot -Tpng -o rpn.png rpn.dot
//   chrome rpn.png
//

package main

import (
	"os"
	"fmt"
	"strconv"
)

type stack struct {
	items []int
	count int
}

func (s *stack) pop() int {
	s.count--
	v := s.items[s.count]
	return v
}

func (s *stack) push(v int) {
	s.items[s.count] = v
	s.count++
}

func abs(v int) int {
	if v < 0 {
		v = -v
	}
	return v
}

%% machine rpn;
%% write data;

func rpn(data string) (res int, err os.Error) {
	// p, pe, eof := 0, len(data), len(data)
	cs, p, pe := 0, 0, len(data)
	mark := 0
	st := &stack{items: make([]int, 128), count: 0}

	%%{
		action mark { mark = p }
		action push { x, _ := strconv.Atoi(data[mark:p]); st.push(x) }
		action add  { y, x := st.pop(), st.pop(); st.push(x + y) }
		action sub  { y, x := st.pop(), st.pop(); st.push(x - y) }
		action mul  { y, x := st.pop(), st.pop(); st.push(x * y) }
		action div  { y, x := st.pop(), st.pop(); st.push(x / y) }
		action abs  { st.push(abs(st.pop())) }
		action abba { st.push(666) }

		stuff  = digit+ >mark %push
		       | '+' @add
		       | '-' @sub
		       | '*' @mul
		       | '/' @div
		       | 'abs' %abs
		       | 'add' %add
		       | 'abba' %abba
		       ;

		main := ( space | stuff space )* ;

		write init;
		write exec;
	}%%

	if cs < rpn_first_final {
		if p == pe {
			return 0, os.ErrorString("unexpected eof")
		} else {
			return 0, os.ErrorString(fmt.Sprintf("error at position %d", p))
		}
	}

	if st.count == 0 {
		return 0, os.ErrorString("rpn stack empty on result")
	}

	return st.pop(), nil
}

//////////////////////////////////////////////////////////////////////

type rpnTest struct {
	s string
	v int
}

var rpnTests = []rpnTest{
	rpnTest{"666\n", 666},
	rpnTest{"666 111\n", 111},
	rpnTest{"4 3 add\n", 7},
	rpnTest{"4 3 +\n", 7},
	rpnTest{"4 3 -\n", 1},
	rpnTest{"4 3 *\n", 12},
	rpnTest{"6 2 /\n", 3},
	rpnTest{"0 3 -\n", -3},
	rpnTest{"0 3 - abs\n", 3},
	rpnTest{" 2  2 + 3 - \n", 1},
	rpnTest{"10 7 3 2 * - +\n", 11},
	rpnTest{"abba abba add\n", 1332},
}

type rpnFailTest struct {
	s string
	e string
}

var rpnFailTests = []rpnFailTest{
	rpnFailTest{"\n", "rpn stack empty on result"},
}

func main() {
	rc := 0

	for _, test := range rpnTests {
		res, err := rpn(test.s)
		if err != nil {
			fmt.Fprintf(os.Stderr, "FAIL rpn(%#v) %s\n", test.s, err)
			rc = 1
		} else if res != test.v {
			fmt.Fprintf(os.Stderr, "FAIL rpn(%#v) -> %#v != %#v\n",
				test.s, res, test.v)
			rc = 1
		}
	}

	for _, test := range rpnFailTests {
		res, err := rpn(test.s)
		if err == nil {
			fmt.Fprintf(os.Stderr, "FAIL rpn(%#v) -> %#v should fail: %#v\n",
				test.s, res, test.e)
		} else if err.String() != test.e {
			fmt.Fprintf(os.Stderr, "FAIL rpn(%#v) %#v should be %#v\n",
				test.s, err.String(), test.e)
		}
	}

	os.Exit(rc)
}
