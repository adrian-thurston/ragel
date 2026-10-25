/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * colm.d: each case is a colm program followed by sections:
 *
 *   ###### ARGS #####   program arguments
 *   ###### COMP ######  compilation arguments
 *   ###### COMP_ERR ### expected compile error
 *   ###### IN #####     program input
 *   ###### EXP #####    expected output
 *   ###### EXIT ######  expected exit value
 *   ###### LOST ######  known leaks: runtime reports that don't fail the run
 *   ###### HOST ######  host program
 *   ###### CALL ######  file containing C functions
 *
 * ARGS, IN, EXP, EXIT and LOST repeat: the Nth of each describes the Nth run
 * of the compiled program. A line ending in --noeol is emitted without its
 * newline.
 *
 * HOST is C++. The manual suite's NAME.host.c is a C host, built without the
 * export header and code. The manual suite's NAME.reducer.h is a program's
 * reducer.h: the program is compiled with -m and its commit code linked in.
 *
 * A case with COMP_ERR is a program colm must reject. It passes when colm
 * exits with 1 and its error output is the section, once the path colm
 * compiles the program from, working/NAME.lm, is taken off the front of each
 * line: "2:9: cannot resolve qualification push". Colm must also leave the
 * program, working/NAME.lm, as it was. It has no runs, so it takes no ARGS,
 * IN, EXP, EXIT or LOST, and no HOST.
 *
 * Each run has COLM_LEAK_CHECK set, so the runtime reports the kids, trees,
 * parse trees, heads and locations the program never freed, one per line as
 * "message: warning: lost trees: 2". A report fails the case unless the rest
 * of its line, "lost trees: 2", is a line of the run's LOST section. With
 * --valgrind each run is under valgrind, and a memory error or definite leak
 * fails the case. Valgrind sees inside the pools only when the runtime is
 * configured with --enable-pool-malloc, which leaves the runtime's own counts
 * at zero.
 *
 * colmCompile and colmRun build these steps for the manual suite too.
 */

#include "harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* The exit value valgrind gives a run in which it found errors. */
static const int valgrindExit = 99;

static std::string noEol( const std::string &body )
{
	std::string out;
	size_t pos = 0;
	while ( pos < body.size() ) {
		size_t eol = body.find( '\n', pos );
		if ( eol == std::string::npos )
			eol = body.size();
		std::string line = body.substr( pos, eol - pos );
		if ( hasSuffix( line, "--noeol" ) ) {
			out += stripSuffix( line, "--noeol" );
		}
		else {
			out += line;
			out += "\n";
		}
		pos = eol + 1;
	}
	return out;
}

static bool section( const CaseFile &cf, const char *name, int nth, std::string &out )
{
	const Section *s = cf.find( name, nth );
	if ( s == 0 )
		return false;
	out = noEol( s->body );
	return true;
}

/* Out of tree, cases open their input and include files, and open1 its own
 * source, relative to the build directory they run in. Link them in. */
static void stage( const std::string &src, const std::string &build )
{
	if ( src == build )
		return;
	std::vector<std::string> names;
	listDir( src, names );
	for ( size_t i = 0; i < names.size(); i++ ) {
		if ( hasSuffix( names[i], ".lm" ) || hasSuffix( names[i], ".lmi" ) ||
				hasSuffix( names[i], ".in" ) )
		{
			std::string target = joinPath( build, names[i] );
			if ( !fileExists( target ) ) {
				if ( symlink( joinPath( src, names[i] ).c_str(), target.c_str() ) != 0 )
					fprintf( stderr, "harness: cannot link %s\n", target.c_str() );
			}
		}
	}
}

/* Write the program to working/ROOT.lm in its suite's build directory and
 * compile it to working/ROOT, ROOT being the job's name. Paths are relative to
 * the build directory the steps run in, as colm.d's cases themselves expect. */
void colmCompile( const Config &config, Job *job, const ColmProgram &prog )
{
	const std::string &root = job->name;
	std::string build = config.suiteBuild( job->suite.c_str() );
	std::string wk = config.working( job->suite.c_str() );
	std::string lm = "working/" + root + ".lm";

	job->steps.push_back( Step::writeFile( joinPath( wk, root + ".lm" ), prog.text ) );

	Words adds;
	if ( prog.hasCall ) {
		job->steps.push_back( Step::writeFile( joinPath( wk, root + ".call.c" ), prog.call ) );
		adds.push_back( "-a" );
		adds.push_back( "working/" + root + ".call.c" );
	}

	if ( prog.hasHost ) {
		std::string hostName = root + ( prog.hostIsC ? ".host.c" : ".host.cc" );
		std::string host = "working/" + hostName;
		job->steps.push_back( Step::writeFile( joinPath( wk, hostName ), prog.host ) );

		std::string parse = "working/" + root + ".parse";
		std::string iface = "working/" + root + ".if";
		std::string commit = "working/" + root + ".commit";
		std::string inc = "working/" + root + ".inc";

		if ( prog.hasReducer ) {
			Words mk;
			mk.push_back( "mkdir" );
			mk.push_back( "-p" );
			mk.push_back( inc );
			job->steps.push_back( Step::exec( Step::Compile, mk, build ) );
			job->steps.push_back( Step::writeFile(
					joinPath( wk, root + ".inc/reducer.h" ), prog.reducer ) );
			job->artifacts.push_back( joinPath( wk, root + ".inc/reducer.h" ) );
		}

		Words argv;
		argv.push_back( config.colmBin );
		argv.insert( argv.end(), prog.comp.begin(), prog.comp.end() );
		argv.push_back( "-c" );
		argv.push_back( "-o" );
		argv.push_back( parse + ".c" );
		if ( !prog.hostIsC ) {
			argv.push_back( "-e" );
			argv.push_back( iface + ".h" );
			argv.push_back( "-x" );
			argv.push_back( iface + ".cc" );
		}
		if ( prog.hasReducer ) {
			argv.push_back( "-m" );
			argv.push_back( commit + ".cc" );
		}
		argv.push_back( lm );
		job->steps.push_back( Step::exec( Step::Compile, argv, build ).capture( CaptureLog ) );

		argv.clear();
		argv.push_back( config.cc );
		argv.push_back( "-c" );
		argv.insert( argv.end(), config.colmCppflags.begin(), config.colmCppflags.end() );
		argv.insert( argv.end(), config.colmLdflags.begin(), config.colmLdflags.end() );
		argv.push_back( "-o" );
		argv.push_back( parse + ".o" );
		argv.push_back( parse + ".c" );
		job->steps.push_back( Step::exec( Step::Compile, argv, build ).capture( CaptureLog ) );

		if ( prog.hasReducer ) {
			argv.clear();
			argv.push_back( config.cxx );
			argv.push_back( "-c" );
			argv.push_back( "-I" + inc );
			argv.insert( argv.end(), config.colmCppflags.begin(), config.colmCppflags.end() );
			argv.push_back( "-o" );
			argv.push_back( commit + ".o" );
			argv.push_back( commit + ".cc" );
			job->steps.push_back( Step::exec( Step::Compile, argv, build ).capture( CaptureLog ) );
		}

		argv.clear();
		argv.push_back( prog.hostIsC ? config.cc : config.cxx );
		argv.push_back( "-I." );
		if ( prog.hasReducer )
			argv.push_back( "-I" + inc );
		argv.insert( argv.end(), config.colmCppflags.begin(), config.colmCppflags.end() );
		argv.insert( argv.end(), config.colmLdflags.begin(), config.colmLdflags.end() );
		argv.push_back( "-o" );
		argv.push_back( "working/" + root );
		if ( !prog.hostIsC )
			argv.push_back( iface + ".cc" );
		argv.push_back( host );
		argv.push_back( parse + ".o" );
		if ( prog.hasReducer )
			argv.push_back( commit + ".o" );
		argv.push_back( "-lcolm" );
		job->steps.push_back( Step::exec( Step::Compile, argv, build ).capture( CaptureLog ) );
	}
	else {
		Words argv;
		argv.push_back( config.colmBin );
		argv.push_back( "-B" );
		argv.push_back( config.topBuilddir );
		argv.insert( argv.end(), prog.comp.begin(), prog.comp.end() );
		argv.insert( argv.end(), adds.begin(), adds.end() );
		argv.push_back( lm );

		Step compile = Step::exec( Step::Compile, argv, build ).capture( CaptureLog );
		if ( prog.hasCompErr ) {
			/* Colm's errors are the output, without the path in front. */
			std::string path = lm;
			replaceAll( path, ".", "\\." );
			job->steps.push_back( compile.exit( 1 ).stderrOutput() );
			job->steps.push_back( Step::filter( "sed -e 's|^" + path + ":||'", build ) );
			job->steps.push_back( Step::compare( prog.compErr, "compile error" ) );

			/* Colm writes none of its output over the program it rejects. */
			Words cat;
			cat.push_back( "cat" );
			cat.push_back( lm );
			Step read = Step::exec( Step::Compile, cat, build ).capture( CaptureOutput );
			job->steps.push_back( read );
			job->steps.push_back( Step::compare( prog.text, "program" ) );
		}
		else {
			job->steps.push_back( compile );
		}
	}

	job->artifacts.push_back( joinPath( wk, root ) );
	job->artifacts.push_back( joinPath( wk, root + ".*" ) );
	job->artifacts.push_back( joinPath( wk, root + "-*" ) );
}

/* Run the program colmCompile built, with COLM_LEAK_CHECK set, and compare its
 * output. */
void colmRun( const Config &config, Job *job, const ColmRun &run )
{
	std::string build = config.suiteBuild( job->suite.c_str() );

	int errorExit = -1;
	Words argv;
	if ( config.valgrind ) {
		errorExit = valgrindExit;
		char opt[64];
		snprintf( opt, sizeof(opt), "--error-exitcode=%d", valgrindExit );
		argv.push_back( "valgrind" );
		argv.push_back( "-q" );
		argv.push_back( opt );
		argv.push_back( "--leak-check=full" );
		argv.push_back( "--show-leak-kinds=definite" );
		argv.push_back( "--errors-for-leak-kinds=definite" );
	}
	argv.push_back( "./working/" + job->name );
	argv.insert( argv.end(), run.args.begin(), run.args.end() );

	job->steps.push_back( Step::exec( Step::Run, argv, build )
			.stdinFrom( run.stdinFile ).capture( CaptureOutput ).exit( run.exitValue )
			.environment( "COLM_LEAK_CHECK=1" )
			.errorsFrom( errorExit, "message: warning: ", run.lost ).labelled( run.label ) );
	job->steps.push_back( Step::compare( run.expected, run.label ) );
}

void enumerateColm( const Config &config, const Selection &sel, JobList &jobs )
{
	const char *suite = "colm.d";
	std::string src = config.suiteSrc( suite );
	std::string build = config.suiteBuild( suite );
	std::string wk = config.working( suite );

	if ( !config.list )
		stage( src, build );

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

		CaseFile cf;
		std::string err;
		if ( !cf.load( joinPath( src, names[i] ), false, err ) ) {
			job->error( err );
			continue;
		}

		ColmProgram prog;
		prog.text = noEol( cf.preamble );
		prog.hasCall = section( cf, "CALL", 0, prog.call );
		prog.hasHost = section( cf, "HOST", 0, prog.host );
		prog.hasCompErr = section( cf, "COMP_ERR", 0, prog.compErr );

		std::string body;
		if ( section( cf, "COMP", 0, body ) )
			prog.comp = splitWords( body );

		if ( prog.hasCompErr ) {
			static const char *noRuns[] = { "ARGS", "IN", "EXP", "EXIT", "LOST", "HOST", 0 };
			for ( const char **name = noRuns; *name != 0; name++ ) {
				if ( cf.has( *name ) ) {
					job->error( std::string( "a COMP_ERR case takes no " ) + *name );
					break;
				}
			}
			if ( job->outcome == Pending )
				colmCompile( config, job, prog );
			continue;
		}

		colmCompile( config, job, prog );

		/* One run per expected output. With none at all, one run against an
		 * empty expected output. */
		int total = 0;
		for ( int nth = 0; ; nth++ ) {
			if ( cf.find( "EXP", nth ) == 0 && nth > 0 )
				break;
			total++;
			if ( cf.find( "EXP", nth ) == 0 )
				break;
		}

		for ( int nth = 0; nth < total; nth++ ) {
			ColmRun run;
			section( cf, "EXP", nth, run.expected );

			if ( section( cf, "ARGS", nth, body ) )
				run.args = splitWords( body );

			if ( section( cf, "IN", nth, body ) ) {
				char num[32];
				snprintf( num, sizeof(num), "-%d.in", nth );
				run.stdinFile = joinPath( wk, root + num );
				job->steps.push_back( Step::writeFile( run.stdinFile, body ) );
			}
			else if ( fileExists( joinPath( src, root + ".in" ) ) ) {
				run.stdinFile = joinPath( src, root + ".in" );
			}

			if ( section( cf, "EXIT", nth, body ) && !trim( body ).empty() )
				run.exitValue = atoi( trim( body ).c_str() );

			section( cf, "LOST", nth, run.lost );

			if ( total > 1 ) {
				char num[32];
				snprintf( num, sizeof(num), "run %d", nth );
				run.label = num;
			}

			colmRun( config, job, run );
		}
	}
}
