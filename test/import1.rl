/*
 * @LANG: c
 */

#include <stdio.h>

char *foo = "foo";

char b = 98;
char a = 97;
char r = 114;

#define SP 32
#define NL '\n'

%%{
	machine tmp;
	import "import1.rl";

	foobar = 
		foo @{printf("foo\n"); } |
		b a r @{printf("bar\n");};

	main := ( foobar SP foobar NL )*;
}%%

%% write data;

int cs;

void exec_str( char *p, int len )
{
	char *pe = p + len;
	%% write exec;
}

void exec_c( char c )
{
	exec_str( &c, 1 );
}

int main()
{
	%% write init;

	exec_str( foo, 3 );
	exec_c( SP );
	exec_c( b );
	exec_c( a );
	exec_c( r );
	exec_c( NL );

	exec_c( b );
	exec_c( a );
	exec_c( r );
	exec_c( SP );
	exec_str( foo, 3 );
	exec_c( NL );

	if ( cs < tmp_first_final )
		printf("FAIL\n");
	else
		printf("ACCEPT\n");

	return 0;
}
#ifdef _____OUTPUT_____
foo
bar
bar
foo
ACCEPT
#endif
