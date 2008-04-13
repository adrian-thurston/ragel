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

	m1 = ( "" %{printf("enter m1 aa\n");} |
			'aa'* >{printf("enter m1 aa\n");} %{printf("leave m1 aa\n");} )
		'b' @{printf("through m1 b\n");} . 'b'* . 'a'*;

	m2 = 'bbb'* 'aa'*;

	main := ( 
			m1 %{printf("accept m1\n");} |
			"" %{printf("enter m2\n");} |
			m2 >{printf("enter m2\n");} %{printf("accept m2\n");}
		) . '\n';
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
	int len = strlen( buf );
	forder_init( &fsm );
	forder_execute( &fsm, buf, len );
	if ( forder_finish( &fsm ) > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}

int main()
{
	test( "aaaaaabbbaa\n" );
	test( "\n" );
	test( "bbbbbbaaaaaaa\n" );
	test( "bbbbbbaaaaaa\n" );
	test( "aaaaa\n" );

	return 0;
}

#ifdef _____OUTPUT_____
enter m1 aa
enter m2
leave m1 aa
through m1 b
accept m1
ACCEPT
enter m2
enter m2
accept m2
ACCEPT
enter m1 aa
enter m1 aa
leave m1 aa
through m1 b
enter m2
accept m1
ACCEPT
enter m1 aa
enter m1 aa
leave m1 aa
through m1 b
enter m2
accept m1
accept m2
ACCEPT
enter m1 aa
enter m2
FAIL
#endif
