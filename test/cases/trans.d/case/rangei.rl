/*
 * @LANG: indep
 */
%%{
	machine rangei;

	main := 
		'a' ../i 'z' .
		'A' ../i 'Z' .
		60 ../i 93 .
		94 ../i 125 .
		86 ../i 101 .
		60 ../i 125
		'';
}%%

##### INPUT #####
"AaBbAa"
"Aa`bAa"
"AaB@Aa"
"AaBbMa"
"AaBbma"
##### OUTPUT #####
ACCEPT
FAIL
FAIL
FAIL
FAIL
