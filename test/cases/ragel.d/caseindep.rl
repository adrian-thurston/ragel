/*
 * @LANG: indep
 */
%%{
	machine indep;

	main := (
		( '1' 'hello' ) |
		( '2' "hello" ) |
		( '3' /hello/ ) |
		( '4' 'hello'i ) |
		( '5' "hello"i ) |
		( '6' /hello/i )
	) '\n';
}%%

##### INPUT #####
"1hello\n"
"1HELLO\n"
"1HeLLo\n"
"2hello\n"
"2HELLO\n"
"2HeLLo\n"
"3hello\n"
"3HELLO\n"
"3HeLLo\n"
"4hello\n"
"4HELLO\n"
"4HeLLo\n"
"5hello\n"
"5HELLO\n"
"5HeLLo\n"
"6hello\n"
"6HELLO\n"
"6HeLLo\n"
##### OUTPUT #####
ACCEPT
FAIL
FAIL
ACCEPT
FAIL
FAIL
ACCEPT
FAIL
FAIL
ACCEPT
ACCEPT
ACCEPT
ACCEPT
ACCEPT
ACCEPT
ACCEPT
ACCEPT
ACCEPT
