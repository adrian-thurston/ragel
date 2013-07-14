#include "load.h"
#include "if.h"
#include "ragel.h"
#include "inputdata.h"

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
	LoadRagel( InputData &id )
	:
		id(id),
		machineSpec(0),
		machineName(0),
		includeDepth(0)
	{}

	InputData &id;
	char *machineSpec;
	char *machineName;
	int includeDepth;

	void loadSection( c_host::section Section )
	{
		switch ( Section.prodName() ) {
			case c_host::section::_SingleLine:
			case c_host::section::_MultiLine:
				break;

			case c_host::section::_Tok:
				/* If no errors and we are at the bottom of the include stack (the
				 * source file listed on the command line) then write out the data. */
				if ( includeDepth == 0 && machineSpec == 0 && machineName == 0 ) {
					string text = Section.Tok().text();
					id.inputItems.tail->data << text;
				}

				break;
		}
	}

	void load( start Start )
	{
		c_host::_repeat_section SectionList = Start.SectionList();
		while ( !SectionList.end() ) {
			loadSection( SectionList.value() );
			SectionList = SectionList.next();
		}
	}

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

		load( Start );

		colm_delete_program( program );
	}
};

LoadRagel *newLoadRagel( InputData &id )
{
	return new LoadRagel( id );
}

void loadRagel( LoadRagel *lr, const char *inputFileName )
{
	lr->load( inputFileName );
}

void deleteLoadRagel( LoadRagel *lr )
{
	delete lr;
}
