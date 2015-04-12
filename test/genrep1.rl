/*
 * @LANG: c
 * @PROHIBIT_GENFLAGS: -T0 -T1 -F0 -G0 -G1 -G2
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


long c;

struct nfa_state_rec
{
	long c;
};

struct nfa_state_rec nfa_s[1024];

void nfa_push()
{
	printf( "push\n" );
	nfa_s[nfa_len].c = c;
}

void nfa_pop()
{
	printf( "pop\n" );
	c = nfa_s[nfa_len].c;
}

%%{
	machine match_any;
	alphtype unsigned char;

	action eol { p+1 == eof }

	eol = '' %when eol;

	action ini6
	{
		printf( "ini_6\n" );
		c = 0;
	}

	action min6
	{
		printf( "min_6\n" );
		if ( ++c < 2 )
			fgoto *match_any_error;
	}

	action max6
	{
		printf( "max_6\n" );
		if ( ++c == 3 )
			fgoto *match_any_error;
	}

	main := 
		( :( ( 'a' @{printf("b\n");} ),
				ini6, min6, max6, {nfa_push();}, {nfa_pop();} ): ' ' ) {2}
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

	%%write init;
	%%write exec;
	
	return 0;
}

int main()
{
	test( "aaa aaa " );
	return 0;
}

##### OUTPUT #####
push
pop
ini_6
b
push
push
push
pop
min_6
pop
max_6
b
push
push
push
pop
min_6
pop
max_6
b
push
push
push
pop
min_6
push
pop
ini_6
b
push
push
push
pop
min_6
pop
max_6
b
push
push
push
pop
min_6
pop
max_6
b
push
push
push
pop
min_6
----- MATCH
pop
max_6
pop
pop
pop
pop
max_6
pop
pop
pop
