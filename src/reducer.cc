#include "reducer.h"

using std::endl;


void Reducer::loadMachineName( string data )
{
	/* Make/get the priority key. The name may have already been referenced
	 * and therefore exist. */
	PriorDictEl *priorDictEl;
	if ( pd->priorDict.insert( data, pd->nextPriorKey, &priorDictEl ) )
		pd->nextPriorKey += 1;
	pd->curDefPriorKey = priorDictEl->value;

	/* Make/get the local error key. */
	LocalErrDictEl *localErrDictEl;
	if ( pd->localErrDict.insert( data, pd->nextLocalErrKey, &localErrDictEl ) )
		pd->nextLocalErrKey += 1;
	pd->curDefLocalErrKey = localErrDictEl->value;
}

void Reducer::tryMachineDef( InputLoc &loc, std::string name, 
		MachineDef *machineDef, bool isInstance )
{
	GraphDictEl *newEl = pd->graphDict.insert( name );
	if ( newEl != 0 ) {
		/* New element in the dict, all good. */
		newEl->value = new VarDef( name, machineDef );
		newEl->isInstance = isInstance;
		newEl->loc = loc;
		newEl->value->isExport = exportContext[exportContext.length()-1];

		/* It it is an instance, put on the instance list. */
		if ( isInstance ) {
			pd->instanceList.append( newEl );

			InputItem *inputItem = new InputItem;
			inputItem->type = InputItem::EndSection;
			section->lastReference = inputItem;
			id.inputItems.append( inputItem );
		}
	}
	else {
		// Recover by ignoring the duplicate.
		error(loc) << "fsm \"" << name << "\" previously defined" << endl;
	}
}

void Reducer::reduceFile( const char *inputFileName,
		const char *targetMachine, const char *searchMachine )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "reduce";
	argv[2] = inputFileName;
	argv[3] = id.hostLang->rlhcArg;
	argv[4] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, 0 );
	colm_run_program( program, 4, argv );

	colm_delete_program( program );
}
