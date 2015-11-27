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
		({ ++q_2 < 1; })
	}

	action exit_2 	{
		({ ++q_2 >= 1; })
	}

	action leaving
	{
		printf( "  -> leaving\n" );
	}

	action c1 { ({ printf( "  -> c1\n"); c1; }) }
	action c2 { ({ printf( "  -> c2\n"); c2; }) }
 
	main :=
		(
			( 
				'hello' %when c1 |
				'hello' %when c2
			)

			:nfa( 1, ( ' ' ), psh, pop, ini_2, stay_2, repeat_2, exit_2 ):

			'there'
		)
		:>
		any @{
			printf( "------ MATCH\n" );
		};

	write data;
}%%

int test( int c1, int c2, const char *p )
{
	int len = strlen( p ) + 1;
	const char *pe = p + len;
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
	test( 0, 0, "hellothere" );
	test( 0, 0, "hello there" );
	test( 0, 0, "hello  there" );

	printf( "------------\n" );

	test( 0, 1, "hellothere" );
	test( 0, 1, "hello there" );
	test( 0, 1, "hello  there" );

	printf( "------------\n" );

	test( 1, 0, "hellothere" );
	test( 1, 0, "hello there" );
	test( 1, 0, "hello  there" );

	printf( "------------\n" );

	test( 1, 1, "hellothere" );
	test( 1, 1, "hello there" );
	test( 1, 1, "hello  there" );

	return 0;
}

########## OUTPUT ##########
testing: hellothere
  -> c1
  -> c2
testing: hello there
  -> c1
  -> c2
testing: hello  there
  -> c1
  -> c2
------------
testing: hellothere
  -> c1
  -> c2
testing: hello there
  -> c1
  -> c2
------ MATCH
testing: hello  there
  -> c1
  -> c2
------------
testing: hellothere
  -> c1
  -> c2
testing: hello there
  -> c1
  -> c2
------ MATCH
testing: hello  there
  -> c1
  -> c2
------------
testing: hellothere
  -> c1
  -> c2
testing: hello there
  -> c1
  -> c2
------ MATCH
testing: hello  there
  -> c1
  -> c2
