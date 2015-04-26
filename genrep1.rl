/*
 * @LANG: c
 * @PROHIBIT_GENFLAGS: -T0 -T1 -G0 -G1 -G2
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
	int pop;
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

%%{
	machine match_any;
	alphtype unsigned char;

	action eol { p+1 == eof }

	eol = '' %when eol;

	action ini6
	{
		c = 0;
	}

	action min6
	{
		if ( ++c < 2 )
			fgoto *match_any_error;
	}

	action max6
	{
		if ( ++c == 3 )
			fgoto *match_any_error;
	}

	main := 
		( :( ( 'a' ), ini6, min6, max6, {nfa_push();}, {nfa_pop();} ): ' ' ) {2}
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

	printf( "testing: %s\n", data );

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
testing: a 
testing: aa 
testing: aaa 
testing: aaaa 
testing: a a 
testing: aa aa 
----- MATCH
testing: aaa aaa 
----- MATCH
testing: aaaa aaaa 
testing: a a a 
testing: aa aa aa 
testing: aaa aaa aaa 
testing: aaaa aaaa aaaa 
testing: aa a 
testing: aa aaa 
----- MATCH
testing: aa aaaa 
testing: aaa a 
testing: aaa aa 
----- MATCH
testing: aaa aaaa 
