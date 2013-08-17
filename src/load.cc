#include "load.h"
#include "if.h"
#include "ragel.h"
#include "inputdata.h"
#include "parsedata.h"

#include <colm/colm.h>
#include <colm/tree.h>

using std::endl;

extern colm_sections colm_object;

InputLoc::InputLoc( colm_location *pcloc )
{
	if ( pcloc != 0 ) {
		fileName = pcloc->name;
		line = pcloc->line;
		col = pcloc->column;
	}
	else {
		fileName = 0;
		line = -1;
		col = -1;
	}
}

struct LoadRagel
{
	LoadRagel( InputData &id )
	:
		id(id),
		pd(0),
		machineSpec(0),
		machineName(0),
		includeDepth(0)
	{}

	InputData &id;
	ParseData *pd;
	char *machineSpec;
	char *machineName;
	int includeDepth;

	void loadActionSpec( ragel::action_spec ActionSpec )
	{
		std::cerr << "loading: " << ActionSpec.Name().text() <<
				" - " << ActionSpec.ActionBlock().InlineList().text() << std::endl;

		ragel::word Name = ActionSpec.Name();
		InputLoc loc = Name.loc();
		colm_data *name = Name.data();
		InlineList *inlineList = 0;

		if ( pd->actionDict.find( name->data ) ) {
			/* Recover by just ignoring the duplicate. */
			error(loc) << "action \"" << name << "\" already defined" << endl;
		}
		else {
			/* Add the action to the list of actions. */
			Action *newAction = new Action( loc, name->data, 
					inlineList, pd->nextCondId++ );

			/* Insert to list and dict. */
			pd->actionList.append( newAction );
			pd->actionDict.insert( newAction );
		}
	}

	void loadStatement( ragel::statement Statement )
	{
		switch( Statement.prodName() ) {
			case ragel::statement::_ActionSpec: {
				loadActionSpec( Statement.ActionSpec() );
				break;
			}
		}
	}

	void loadStmtList( ragel::_repeat_statement StmtList )
	{
		while ( !StmtList.end() ) {
			loadStatement( StmtList.value() );
			StmtList = StmtList.next();
		}
	}

	void loadSection( c_host::section Section )
	{
		switch ( Section.prodName() ) {
			case c_host::section::_MultiLine:
				loadStmtList( Section.StmtList() );
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
		InputLoc loc;
		pd = new ParseData( "foo", "bar", loc );

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
			InputLoc loc( Error.loc() );
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
