/*
 * @LANG: c
 * @ALLOW_GENFLAGS: -T0 -T1 -G0 -G1 -G2
 */

/**
 * Test a high character to make sure signedness 
 * isn't messing us up.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct high
{
	int cs;
};

%%{
	machine high;
	variable cs fsm->cs;

	# We Want the header portion.
	alphtype unsigned int;

	main := ( 
			0x20 .. 0xefffffff   @1 @{printf("gothigh1\n");} | 
			0xf0000000           @1 @{printf("gothigh1\n");} | 
			0x200 .. 0xfe000000  @1 @{printf("gothigh2\n");} |
			any                  @0 @{printf("else\n");}
		)*;
}%%

%% write data;

void high_init( struct high *fsm )
{
	%% write init;
}

void high_execute( struct high *fsm, const unsigned int *_data, int _len )
{
	const unsigned int *p = _data;
	const unsigned int *pe = _data+_len;

	%% write exec;
}

int high_finish( struct high *fsm )
{
	if ( fsm->cs == high_error )
		return -1;
	if ( fsm->cs >= high_first_final )
		return 1;
	return 0;
}

struct high high;

#define BUFSIZE 1024
char cbuf[BUFSIZE];
unsigned int buf[BUFSIZE];
int buflen = 0;
char numbuf[9];
int numlen = 0;

struct tokenizer
{
	int cs;
};

%%{
	machine tokenizer;
	variable cs fsm->cs;

	action bufdigit {
		if ( numlen < 8 )
			numbuf[numlen++] = fc;
	}

	action writeDigit {
		/* Null terminate the buffer storing the number and reset. */
		numbuf[numlen] = 0;
		numlen = 0;

		/* Store the number in the buf. If the buf is full then 
		 * flush and reset the buffer. */
		buf[buflen++] = strtoul( numbuf, 0, 16 );
		if ( buflen == BUFSIZE ) {
			high_execute( &high, buf, BUFSIZE );
			buflen = 0;
		}
	}

	action finish {
		if ( buflen > 0 )
			high_execute( &high, buf, buflen );
		if ( high_finish( &high ) > 0 )
			printf("ACCEPT\n");
		else
			printf("FAIL\n");
	}

	num = ( digit | 'a'..'f' )+ $bufdigit %writeDigit;
	main := ( num $1 %0 | space )* %/finish;
}%%

%% write data;

void tokenizer_init( struct tokenizer *fsm )
{
	%% write init;
}

void tokenizer_execute( struct tokenizer *fsm, const char *_data, int _len )
{
	const char *p = _data;
	const char *pe = _data+_len;
	const char *eof = pe;

	%% write exec;
}

int tokenizer_finish( struct tokenizer *fsm )
{
	if ( fsm->cs == tokenizer_error )
		return -1;
	if ( fsm->cs >= tokenizer_first_final )
		return 1;
	return 0;
}

struct tokenizer tok;

void test( char *cbuf )
{
	int len = strlen( cbuf );
	high_init( &high );
	tokenizer_init( &tok );
	tokenizer_execute( &tok, cbuf, len );
	if ( tokenizer_finish( &tok ) <= 0 )
		printf("Tokenizer FAIL\n");
}

char data[] =
	"10 20 30 40 50 200 300 400 \n"
	"d0000000 f0000000 fd000000 fe000000\n"
	"ff000000 ffffffffffffffffffffffffff\n"
	"ff\n";

int main()
{
	test( data );
	return 0;
}

#ifdef _____OUTPUT_____
else
gothigh1
gothigh1
gothigh1
gothigh1
gothigh1
gothigh2
gothigh1
gothigh2
gothigh1
gothigh2
gothigh1
gothigh2
gothigh1
gothigh2
gothigh2
gothigh2
else
else
gothigh1
ACCEPT
#endif
