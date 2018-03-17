/*
 * Copyright 2001-2018 Adrian Thurston <thurston@colm.net>
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

#include "inputdata.h"
#include <colm/colm.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

extern struct colm_sections rlhc;

/* What are the appropriate types for ruby? */
static HostType hostTypesRuby[] = 
{
	{ "char",    0,  "char",   true,   true,  false,  CHAR_MIN,  CHAR_MAX,    0, 0, 1 },
	{ "int",     0,  "int",    true,   true,  false,  INT_MIN,   INT_MAX,     0, 0, 4 },
};

const char *ruby_defaultOutFn( const char *inputFileName )
{
	return fileNameFromStem( inputFileName, ".rb" );
}

static const HostLang hostLangRuby = {
	"Ruby",
	"-R",
	HostLang::Ruby,
	hostTypesRuby, 2,
	hostTypesRuby+0,
	false,
	true,
	"ruby",
	&ruby_defaultOutFn
};

int run_rlhc( int argc, const char **argv )
{
	struct colm_program *prg;
	int exit_status;

	prg = colm_new_program( &rlhc );
	colm_set_debug( prg, 0 );
	colm_run_program( prg, argc, argv );
	exit_status = colm_delete_program( prg );
	return exit_status;
}

int main( int argc, const char **argv )
{
	int status = 0;
	pid_t pid = 0;
	InputData id;

	id.parseArgs( argc, argv );
	id.hostLang = &hostLangRuby;

	id.checkArgs();
	id.makeDefaultFileName();
	id.makeTranslateOutputFileName();

	pid = fork();
	if ( pid == 0 ) {
		id.backend = Translated;
		id.backendSpecified = true;

		/* Child. */
		if ( !id.process() )
			id.abortCompile( 1 );
		exit( 0 );
	}

	waitpid( pid, &status, 0 );

	pid = fork();
	if ( pid == 0 ) {
		/* rlhc <input> <output> */
		const char *_argv[] = { "rlhc",
			id.genOutputFileName.c_str(),
			id.origOutputFileName.c_str(), 0 };
		run_rlhc( 3, _argv );
	}

	waitpid( pid, &status, 0 );
	return 0;
}
