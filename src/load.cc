#include "load.h"
#include "if.h"
#include "ragel.h"
#include <colm/colm.h>
#include <colm/tree.h>

extern colm_sections colm_object;
InputLoc makeInputLoc( const struct colm_location *cl )
{
	InputLoc loc = { "<internal>", cl->line, cl->column };
	return loc;
}

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

		/* Extract the parse tree. */
		start Start = RagelTree( program );
		str Error = RagelError( program );

		if ( Start == 0 ) {
			gblErrorCount += 1;
			InputLoc loc = makeInputLoc( Error.loc() );
			error(loc) << inputFileName << ": parse error: " << Error.text() << std::endl;
			return;
		}

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
