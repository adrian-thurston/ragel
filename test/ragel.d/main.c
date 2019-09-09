#include <colm/colm.h>

extern struct colm_sections trans_object;

int main( int argc, const char **argv )
{
	struct colm_program *prg;
	int exit_status;

	prg = colm_new_program( &trans_object );
	colm_set_debug( prg, 0 );
	colm_run_program( prg, argc, argv );
	exit_status = colm_delete_program( prg );
	return exit_status;
}

