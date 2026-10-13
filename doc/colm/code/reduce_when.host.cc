#include <stdio.h>
#include <colm/colm.h>
#include "reducer.h"

int main( int argc, const char **argv )
{
	Trace trace;

	struct colm_program *prg = colm_new_program( &colm_object );
	colm_set_reduce_ctx( prg, &trace );
	colm_run_program( prg, argc, argv );

	int length;
	const char *error = colm_error( prg, &length );
	if ( error != 0 )
		printf( "parse failed: %.*s\n", length, error );

	return colm_delete_program( prg );
}
