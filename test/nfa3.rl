/*
 * @LANG: c
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h> 
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

const char s[4096];

struct nfa_stack
{
	void *data;
	unsigned long sz;
};

struct nfa_bp_rec
{
	long state;
	const char *p;
	long popTrans;
	long q_2;
};

%%{
	machine atoi;

	action a {a}
	action b {b}

	action psh     { nfa_bp[nfa_len].q_2 = q_2; }
	action pop     { ({ q_2 = nfa_bp[nfa_len].q_2; 1; }) }
	action ini 	   { ({  q_2 = 0; 1; }) }
	action stay    { ({ 1; }) }
	action repeat  { ({ ++q_2 < 2; }) }
	action exit    { ({ ++q_2 >= 2; }) }

	main :=
	(
		( 'a' %when a %when b)
		|
		( 'a' %when a )
	)

	:nfa( ( '.' ), psh, pop, ini, stay, repeat, exit ):

	';'
		@{ printf( " --> MATCH\n" ); };
}%%

%% write data;

int test( const char *p, bool a, bool b )
{
	int len = strlen( p ) + 1;
	const char *pe = p + len;
	int cs;

	struct nfa_bp_rec *nfa_bp = (struct nfa_bp_rec*) s;
	long nfa_len = 0;
	long nfa_count = 0;

	long q_2 = 0;

	printf( "testing: %s %d %d\n", p, a, b );

	%%{
		write init;
		write exec;
	}%%

	return 0;
}

int main()
{
	test( "a.;", 0, 0 );
	test( "a.;", 0, 1 );
	test( "a.;", 1, 0 );
	test( "a.;", 1, 1 );
	test( "a..;", 0, 0 );
	test( "a..;", 0, 1 );
	test( "a..;", 1, 0 );
	test( "a..;", 1, 1 );
	test( "a...;", 0, 0 );
	test( "a...;", 0, 1 );
	test( "a...;", 1, 0 );
	test( "a...;", 1, 1 );
	return 0;
}

##### OUTPUT #####
testing: a.; 0 0
testing: a.; 0 1
testing: a.; 1 0
testing: a.; 1 1
testing: a..; 0 0
testing: a..; 0 1
testing: a..; 1 0
 --> MATCH
testing: a..; 1 1
 --> MATCH
testing: a...; 0 0
testing: a...; 0 1
testing: a...; 1 0
testing: a...; 1 1
