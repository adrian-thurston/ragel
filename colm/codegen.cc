/*
 *  Copyright 2006-2011 Adrian Thurston <thurston@complang.org>
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
#include <sstream>
#include <string>
#include <assert.h>

using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;

void FsmCodeGen::writeMain()
{
	out << 
		"int main( int argc, char **argv )\n"
		"{\n"
		"	initColm( 0 );\n"
		"	Program program;\n"
		"	initProgram( &program, argc, argv, 1, &main_runtimeData );\n"
		"	runProgram( &program );\n"
		"	clearProgram( &program );\n"
		"	return 0;\n"
		"}\n"
		"\n";

	out.flush();
}

