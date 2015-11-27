/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "compiler.h"
#include "fsmcodegen.h"
#include "redfsm.h"
#include "bstmap.h"
#include "debug.h"
#include <sstream>
#include <string>

void FsmCodeGen::writeIncludes()
{
	out << 
		"#include <colm/pdarun.h>\n"
		"#include <colm/debug.h>\n"
		"#include <colm/bytecode.h>\n"
		"#include <colm/config.h>\n"
		"#include <colm/defs.h>\n"
		"#include <colm/input.h>\n"
		"#include <colm/tree.h>\n"
		"#include <colm/program.h>\n"
		"#include <colm/colm.h>\n"
		"\n"
		"#include <stdio.h>\n"
		"#include <stdlib.h>\n"
		"#include <string.h>\n"
		"#include <assert.h>\n"
		"\n"
		"\n";
}


void FsmCodeGen::writeMain( long activeRealm )
{
	out << 
		"int main( int argc, const char **argv )\n"
		"{\n"
		"	struct colm_program *prg;\n"
		"	int exit_status;\n"
		"\n"
		"	prg = colm_new_program( &" << objectName << " );\n"
		"	colm_set_debug( prg, " << activeRealm << " );\n"
		"	colm_run_program( prg, argc, argv );\n"
		"	exit_status = colm_delete_program( prg );\n"
		"	return exit_status;\n"
		"}\n"
		"\n";

	out.flush();
}
