/*
 * Copyright 2006-2012 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>
#include "fsmcodegen.h"

void FsmCodeGen::writeIncludes()
{
	out << 
		"#include <stdio.h>\n"
		"#include <stdlib.h>\n"
		"#include <string.h>\n"
		"#include <assert.h>\n"
		"\n"
		"#include <colm/pdarun.h>\n"
		"#include <colm/debug.h>\n"
		"#include <colm/bytecode.h>\n"
		"#include <colm/config.h>\n"
		"#include <colm/defs.h>\n"
		"#include <colm/input.h>\n"
		"#include <colm/tree.h>\n"
		"#include <colm/program.h>\n"
		"#include <colm/colm.h>\n"
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
