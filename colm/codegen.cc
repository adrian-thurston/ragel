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

#include "parsedata.h"
#include "fsmcodegen.h"
#include "redfsm.h"
#include "bstmap.h"
#include "fsmrun.h"
#include "debug.h"
#include <sstream>
#include <string>


void FsmCodeGen::writeMain()
{
	out << 
		"int main( int argc, const char **argv )\n"
		"{\n"
		"	struct ColmProgram *prg;\n"
		"	int exitStatus;\n"
		"	colmInit( " << colmActiveRealm << " );\n"
		"	prg = colmNewProgram( &main_runtimeData, argc, argv );\n"
		"	colmRunProgram( prg );\n"
		"	exitStatus = colmDeleteProgram( prg );\n"
		"	return exitStatus;\n"
		"}\n"
		"\n";

	out.flush();
}


