/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * manual: the colm manual's example programs, doc/colm/code/NAME.lm. Each is
 * compiled and run as a colm.d case is, with NAME.in, if there is one, on its
 * standard input, and the words of NAME.args, if there is one, as its
 * arguments. Its output is compared against NAME.exp. The chapters include
 * the program and its expected output from these files, so the output they
 * show is the output tested here. The cases live outside test/, but the
 * results go to manual/working under the test build directory, beside the
 * other suites'.
 *
 * Each example is compiled with -I naming doc/colm/code, so it can include a
 * file kept there. Such a file needs a suffix other than .lm, and the
 * chapters use .lmi, since every .lm there is run as an example.
 */

#include "harness.h"

void enumerateManual( const Config &config, const Selection &sel, JobList &jobs )
{
	const char *suite = "manual";
	std::string src = joinPath( config.topSrcdir, "doc/colm/code" );
	std::string wk = config.working( suite );

	std::vector<std::string> names;
	if ( !listCaseDir( suite, src, names, jobs ) )
		return;

	for ( size_t i = 0; i < names.size(); i++ ) {
		if ( !hasSuffix( names[i], ".lm" ) )
			continue;
		if ( !sel.selects( names[i] ) )
			continue;

		std::string root = stripSuffix( names[i], ".lm" );
		Job *job = new Job( suite, root );
		job->reportPath = joinPath( wk, root + ".diff" );
		jobs.append( job );

		ColmProgram prog;
		prog.comp.push_back( "-I" );
		prog.comp.push_back( src );
		if ( !readFile( joinPath( src, names[i] ), prog.text ) ) {
			job->error( "cannot read " + names[i] );
			continue;
		}

		/* An example without expected output is a mistake, not a program
		 * expected to print nothing. */
		ColmRun run;
		if ( !readFile( joinPath( src, root + ".exp" ), run.expected ) ) {
			job->error( "cannot read " + root + ".exp" );
			continue;
		}

		if ( fileExists( joinPath( src, root + ".in" ) ) )
			run.stdinFile = joinPath( src, root + ".in" );

		if ( fileExists( joinPath( src, root + ".args" ) ) ) {
			std::string args;
			if ( !readFile( joinPath( src, root + ".args" ), args ) ) {
				job->error( "cannot read " + root + ".args" );
				continue;
			}
			run.args = splitWords( args );
		}

		colmCompile( config, job, prog );
		colmRun( config, job, run );
	}
}
