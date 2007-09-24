/*
 * @LANG: c
 */

#include <stdio.h>
#include <string.h>

/*
 * After the fact start and ending transitions. Behaves like constructors of
 * and destructors in c++.
 */

struct forder
{
	int cs;
};

%%{
	machine forder;
	variable cs fsm->cs;

	inner = 'inner'
		>{printf("enter inner\n");}
		${printf("inside inner\n");}
		%{printf("leave inner\n");}
	;

	outter = inner
		>{printf("enter outter\n");}
		${printf("inside outter\n");}
		%{printf("leave outter\n");}
	;

	main := outter . '\n';
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
	test( "inner\n");

	test(
		"inner\n"
		"foobar\n"
	);

	test( "" );
	test( "\n" );
	test( "inn\n" );

	return 0;
}

#ifdef _____OUTPUT_____
enter outter
enter inner
inside inner
inside outter
inside inner
inside outter
inside inner
inside outter
inside inner
inside outter
inside inner
inside outter
leave inner
leave outter
ACCEPT
enter outter
enter inner
inside inner
inside outter
inside inner
inside outter
inside inner
inside outter
inside inner
inside outter
inside inner
inside outter
leave inner
leave outter
FAIL
FAIL
FAIL
enter outter
enter inner
inside inner
inside outter
inside inner
inside outter
inside inner
inside outter
FAIL
#endif
