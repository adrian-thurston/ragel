/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
 */

#include "compiler.h"
#include "fsmcodegen.h"
#include "redfsm.h"
#include "bstmap.h"
#include "debug.h"
#include <sstream>
#include <string>


void FsmCodeGen::writeMain( long activeRealm )
{
	out << 
		"int main( int argc, const char **argv )\n"
		"{\n"
		"	struct colm_program *prg;\n"
		"	int exitStatus;\n"
		"	prg = colm_new_program( &colm_object );\n"
		"	colm_set_debug( prg, " << activeRealm << " );\n"
		"	colm_run_program( prg, argc, argv );\n"
		"	exitStatus = colm_delete_program( prg );\n"
		"	return exitStatus;\n"
		"}\n"
		"\n";

	out.flush();
}


