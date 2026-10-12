#include <stdio.h>
#include <colm/colm.h>

extern struct colm_sections words_object;

int main( int argc, const char **argv )
{
	struct colm_program *prg = colm_new_program( &words_object );
	colm_run_program( prg, argc, argv );

	int length;
	const char *error = colm_error( prg, &length );
	if ( error != 0 )
		printf( "parse failed: %.*s\n", length, error );

	int status = colm_delete_program( prg );
	printf( "exit status %d\n", status );
	return status;
}
