/*
 * @LANG: c
 */

/*
 * Test in and out state actions.
 */

#include <stdio.h>
#include <string.h>

struct state_chart
{
	int cs;
};

%%{
	machine state_chart;
	variable cs fsm->cs;

	action a { printf("a"); }
	action b { printf("b"); }
	action hexa { printf("a"); }
	action hexb { printf("b"); }

	hex_a = '0x' '0'* '61' @hexa;
	hex_b = '0x' '0'* '62' @hexb;

	a = 'a' @a | hex_a;
	b = 'b' @b | hex_b;
	ws = ' '+;

	mach = 
		start: ( 
			a -> st1 |
			b -> st2 | 
			zlen -> final 
		),
		st1: ( 
			a -> st1 | 
			ws -> start |
			zlen -> final 
		),
		st2: ( 
			b -> st2 |
			ws -> start |
			zlen -> final
		);
	
	main := ( mach '\n' )*;
}%%

%% write data;

void state_chart_init( struct state_chart *fsm )
{
	%% write init;
}

void state_chart_execute( struct state_chart *fsm, const char *_data, int _len )
{
	const char *p = _data;
	const char *pe = _data+_len;

	%% write exec;
}

int state_chart_finish( struct state_chart *fsm )
{
	if ( fsm->cs == state_chart_error )
		return -1;
	if ( fsm->cs >= state_chart_first_final )
		return 1;
	return 0;
}

struct state_chart sc;

void test( char *buf )
{
	int len = strlen( buf );
	state_chart_init( &sc );
	state_chart_execute( &sc, buf, len );
	state_chart_finish( &sc );
	printf("\n");
}

int main()
{
	test(
		"aa0x0061aa b\n"
		"bbb0x62b 0x61 0x000062\n"
	);

	return 0;
}

#ifdef _____OUTPUT_____
aaaaabbbbbbab
#endif
