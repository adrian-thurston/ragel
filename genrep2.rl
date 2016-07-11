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
	machine genrep;
	alphtype unsigned char;

	action eol { p+1 == eof }
	eol = '' %when eol;

	action psh
	{
		nfa_bp[nfa_len].q_2 = q_2;
	}

	action pop
	{ ({
		q_2 = nfa_bp[nfa_len].q_2;
		1;
	}) }

	action ini_2 	{
		({  q_2 = 0; 1; })
	}

	action stay_2 	{
		({ 1; })
	}

	action repeat_2 	{
		({ ++q_2 < 3; })
	}

	action exit_2 	{
		({ ++q_2 >= 2; })
	}


	main :=
		(
			( :nfa( ( 'a' ) , 
				psh, pop, ini_2, stay_2, repeat_2, exit_2 ): ) {2}
			eol
		)
		:>
		any
		@{ printf( "------ MATCH\n" ); };

	write data;
}%%

int test( const char *p )
{
	int len = strlen( p ) + 1;
	const char *pe = p + len;
	const char *eof = pe;
	int cs;

	struct nfa_bp_rec *nfa_bp = (struct nfa_bp_rec*) s;
	long nfa_len = 0;
	long nfa_count = 0;

	long q_2 = 0;

	printf( "testing: %s\n", p );

	%%{
		machine genrep;
		write init;
		write exec;
	}%%

	return 0;
}

int main()
{
	test( "a" );
	test( "aa" );
	test( "aaa" );
	test( "aaaa" );
	test( "aaaaa" );
	test( "aaaaaa" );
	test( "aaaaaaa" );
	test( "aaaaaaaa" );
	return 0;
}

###### OUTPUT ######
testing: a
testing: aa
testing: aaa
testing: aaaa
------ MATCH
testing: aaaaa
------ MATCH
------ MATCH
testing: aaaaaa
------ MATCH
testing: aaaaaaa
testing: aaaaaaaa
