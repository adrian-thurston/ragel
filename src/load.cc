#include "load.h"
#include "if.h"

extern colm_sections colm_object;

struct LoadRagel
{
	void load( const char *inputFileName )
	{
		const char *argv[2];
		argv[0] = inputFileName;
		argv[1] = 0;

		colm_program *program = colm_new_program( &colm_object );
		colm_set_debug( program, 0 );
		colm_run_program( program, 1, argv );
		colm_delete_program( program );
	}
};

LoadRagel *newLoadRagel()
{
	return new LoadRagel;
}

void loadRagel( LoadRagel *lr, const char *inputFileName )
{
	lr->load( inputFileName );
}

void deleteLoadRagel( LoadRagel *lr )
{
	delete lr;
}


