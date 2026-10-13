/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * manual: the colm manual's example programs, doc/colm/code/NAME.lm. Only the
 * .lm files are cases. Each is compiled and run as a colm.d case is, and the
 * files beside it named after it fill in the rest:
 *
 *   NAME.exp                 the expected output, which every case has
 *   NAME.in                  the program's standard input
 *   NAME.args                the program's arguments, split into words
 *   NAME.comp                colm options, as colm.d's COMP
 *   NAME.host.c, .host.cc    a host program in C or C++, as colm.d's HOST
 *
 * With a host, the program is compiled with -c, and with -e and -x for a C++
 * host, and the host is linked into it. The chapters include the program and
 * its expected output from these files, so the output they show is the output
 * tested here. The cases live outside test/, but the results go to
 * manual/working under the test build directory, beside the other suites'.
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

		if ( fileExists( joinPath( src, root + ".comp" ) ) ) {
			std::string comp;
			if ( !readFile( joinPath( src, root + ".comp" ), comp ) ) {
				job->error( "cannot read " + root + ".comp" );
				continue;
			}
			Words words = splitWords( comp );
			prog.comp.insert( prog.comp.end(), words.begin(), words.end() );
		}

		bool hostC = fileExists( joinPath( src, root + ".host.c" ) );
		bool hostCxx = fileExists( joinPath( src, root + ".host.cc" ) );
		if ( hostC && hostCxx ) {
			job->error( "both " + root + ".host.c and " + root + ".host.cc" );
			continue;
		}
		if ( hostC || hostCxx ) {
			std::string name = root + ( hostC ? ".host.c" : ".host.cc" );
			if ( !readFile( joinPath( src, name ), prog.host ) ) {
				job->error( "cannot read " + name );
				continue;
			}
			prog.hasHost = true;
			prog.hostIsC = hostC;
		}

		colmCompile( config, job, prog );
		colmRun( config, job, run );
	}
}
