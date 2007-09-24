/*
 * @LANG: c
 */

#include <stdio.h>

struct LangEl
{
	int key;
	char *name;
};

struct fsm
{
	int cs;
};

%%{
	machine fsm;
	alphtype int;
	getkey fpc->key;
	variable cs fsm->cs;

	action a1 {}
	action a2 {}
	action a3 {}

	main := ( 1 2* 3  ) 
			${printf("%s\n", fpc->name);} 
			%/{printf("accept\n");};
}%%

%% write data;

void fsm_init( struct fsm *fsm )
{
	%% write init;
}

void fsm_execute( struct fsm *fsm,  struct LangEl *_data, int _len )
{
	struct LangEl *p = _data;
	struct LangEl *pe = _data+_len;
	struct LangEl *eof = pe;

	%% write exec;
}

int fsm_finish( struct fsm *fsm )
{
	if ( fsm->cs == fsm_error )
		return -1;
	if ( fsm->cs >= fsm_first_final )
		return 1;
	return 0;
}

int main()
{
	static struct fsm fsm;
	static struct LangEl lel[] = { 
		{1, "one"}, 
		{2, "two-a"}, 
		{2, "two-b"}, 
		{2, "two-c"}, 
		{3, "three"}
	};

	fsm_init( &fsm );
	fsm_execute( &fsm, lel, 5 );
	fsm_finish( &fsm );

	return 0;
}

#ifdef _____OUTPUT_____
one
two-a
two-b
two-c
three
accept
#endif
