/*
 * @LANG: c
 */

#include <stdio.h>
#include <string.h>

%%{
	machine foo;

	action on_char  { printf("char: %c\n", *p); }
	action on_err   { printf("err: %c\n", *p); }
	action to_state { printf("to state: %c\n", *p); }

	main := 'heXXX' $on_char $err(on_err) $to(to_state);
}%%

%% write data;

int main()
{
	int cs;
	char *p = "hello", *pe = p + strlen(p);
	char *eof = pe;
	%%{
		write init;
		write exec;
	}%%

	printf( "rest: %s\n", p );

	return 0;
}

#ifdef _____OUTPUT_____
char: h
to state: h
char: e
to state: e
err: l
rest: llo
#endif
