/*
 * @LANG: c
 */

#include <stdio.h>
#include <string.h>

struct forder 
{
	int cs;
};

%%{
	machine forder;
	variable cs fsm->cs;

	second = 'b'
		>{printf("enter b1\n");}
		>{printf("enter b2\n");}
	;

	first = 'a'
		%{printf("leave a\n");}
		@{printf("finish a\n");}
	;

	main := first . second . '\n';
}%%

%% write data;

void forder_init( struct forder *fsm )
{
	%% write init;
}

void forder_execute( struct forder *fsm, const char *_data, int _len )
{
	const char *p = _data;
	const char *pe = _data+_len;

	%% write exec;
}

int forder_finish( struct forder *fsm )
{
	if ( fsm->cs == forder_error )
		return -1;
	if ( fsm->cs >= forder_first_final )
		return 1;
	return 0;
}

struct forder fsm;

void test( char *buf )
{
	int len = strlen(buf);
	forder_init( &fsm );
	forder_execute( &fsm, buf, len );
	if ( forder_finish( &fsm ) > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}

int main()
{
	test( "ab\n");
	test( "abx\n");
	test( "" );

	test(
		"ab\n"
		"fail after newline\n"
	);

	return 0;
}

#ifdef _____OUTPUT_____
finish a
leave a
enter b1
enter b2
ACCEPT
finish a
leave a
enter b1
enter b2
FAIL
FAIL
finish a
leave a
enter b1
enter b2
FAIL
#endif
