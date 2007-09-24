/*
 * @LANG: c
 */

#include <stdio.h>
#define IDENT_BUFLEN 256

struct erract
{
	int cs;
};

%%{
	machine erract;
	variable cs fsm->cs;

	# The data that is to go into the fsm structure.
	action hello_fails { printf("hello fails\n");}

	newline = ( any | '\n' @{printf("newline\n");} )*;
	hello = 'hello\n'* $lerr hello_fails @eof hello_fails;
	main := newline | hello;
}%%

%% write data;

void erract_init( struct erract *fsm )
{
	%% write init;
}

void erract_execute( struct erract *fsm, const char *_data, int _len )
{
	const char *p = _data;
	const char *pe = _data+_len;
	const char *eof = pe;
	%% write exec;
}

int erract_finish( struct erract *fsm )
{
	if ( fsm->cs == erract_error )
		return -1;
	else if ( fsm->cs >= erract_first_final )
		return 1;
	return 0;
}

#include <stdio.h>
#include <string.h>

struct erract fsm;

void test( char *buf )
{
	int len = strlen(buf);
	erract_init( &fsm );
	erract_execute( &fsm, buf, len );
	if ( erract_finish( &fsm ) > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}

int main()
{
	test(
		"hello\n"
		"hello\n"
		"hello\n"
	);

	test(
		"hello\n"
		"hello\n"
		"hello there\n"
	);

	test(
		"hello\n"
		"hello\n"
		"he"	);

	test( "" );

	return 0;
}

#ifdef _____OUTPUT_____
newline
newline
newline
ACCEPT
newline
newline
hello fails
newline
ACCEPT
newline
newline
hello fails
ACCEPT
ACCEPT
#endif
