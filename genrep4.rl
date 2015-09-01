/*
 * @LANG: c
 */

#include <stddef.h>  /* NULL */
#include <stdint.h>  /* uint64_t */
#include <stdlib.h>  /* malloc(3) free(3) */
#include <stdbool.h> /* bool */
#include <string.h>
#include <stdio.h>

struct nfa_bp_rec
{
	long state;
	const unsigned char *p;
	int popTrans;
	long q_2;
};

struct nfa_bp_rec nfa_bp[1024];
long nfa_len = 0;
long nfa_count = 0;

long c;

struct nfa_state_rec
{
	long c;
};

struct nfa_state_rec nfa_s[1024];

void nfa_push()
{
	nfa_s[nfa_len].c = c;
}

void nfa_pop()
{
	c = nfa_s[nfa_len].c;
}

long q_2;

%%{
	machine match_any;
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

	action ini 	{
		({  q_2 = 0; 1; })
	}

	action stay 	{
		({ 1; })
	}

	action repeat 	{
		({ ++q_2 < 3; })
	}

	action exit 	{
		({ ++q_2 >= 2; })
	}

	action marker
	{ ({printf("  marker\n");1;}) }

	main := 
		( '' %when(marker) 
			:nfa( 0, ( 'a' ), psh, pop, ini, stay, repeat, exit ): ' ' ) {2}
		eol
		any @{printf("----- MATCH\n");}
	;

	write data;
}%%

int test( const char *data )
{
	int cs;
	const unsigned char *p = (const unsigned char *)data;
	const unsigned char *pe = p + strlen(data) + 1;
	const unsigned char *eof = pe;

	printf( "%s\n", data );

	%% write init;
	%% write exec;
	
	return 0;
}

int main()
{
	test( "a " );
	test( "aa " );
	test( "aaa " );
	test( "aaaa " );

	test( "a a " );
	test( "aa aa " );
	test( "aaa aaa " );
	test( "aaaa aaaa " );

	test( "a a a " );
	test( "aa aa aa " );
	test( "aaa aaa aaa " );
	test( "aaaa aaaa aaaa " );

	test( "aa a " );
	test( "aa aaa " );
	test( "aa aaaa " );

	test( "aaa a " );
	test( "aaa aa " );
	test( "aaa aaaa " );

	return 0;
}

##### OUTPUT #####
a 
  marker
aa 
  marker
  marker
aaa 
  marker
  marker
aaaa 
  marker
a a 
  marker
aa aa 
  marker
  marker
----- MATCH
aaa aaa 
  marker
  marker
----- MATCH
aaaa aaaa 
  marker
a a a 
  marker
aa aa aa 
  marker
  marker
aaa aaa aaa 
  marker
  marker
aaaa aaaa aaaa 
  marker
aa a 
  marker
  marker
aa aaa 
  marker
  marker
----- MATCH
aa aaaa 
  marker
  marker
aaa a 
  marker
  marker
aaa aa 
  marker
  marker
----- MATCH
aaa aaaa 
  marker
  marker
