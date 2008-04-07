/* 
 * @LANG: indep
 * @ALLOW_GENFLAGS: -T0 -T1 -G0 -G1 -G2
 */
bool i;
bool j;
bool k;
%%

%%{
	machine foo;

	action c1 {i}
	action c2 {j}
	action c3 {k}
	action one { prints "  one\n";}
	action two { prints "  two\n";}
	action three { prints "  three\n";}

	action seti { if ( fc == 48 ) i = false; else i = true; }
	action setj { if ( fc == 48 ) j = false; else j = true; }
	action setk { if ( fc == 48 ) k = false; else k = true; }

	action break {fbreak;}

	one = 'a' 'b' when c1 'c' @one;
	two = 'a'* 'b' when c2 'c' @two;
	three = 'a'+ 'b' when c3 'c' @three;

	main := 
		[01] @seti
		[01] @setj
		[01] @setk
		( one | two | three ) '\n' @break;
	
}%%

/* _____INPUT_____ 
"000abc\n"
"100abc\n"
"010abc\n"
"110abc\n"
"001abc\n"
"101abc\n"
"011abc\n"
"111abc\n"
_____INPUT_____ */
/* _____OUTPUT_____
FAIL
  one
ACCEPT
  two
ACCEPT
  one
  two
ACCEPT
  three
ACCEPT
  one
  three
ACCEPT
  two
  three
ACCEPT
  one
  two
  three
ACCEPT
_____OUTPUT_____ */
