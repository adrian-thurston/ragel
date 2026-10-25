/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * cgil.d: translate each case/X.ri with the cgil translator for the language
 * encoded in the file name and compare against the expected file beside it,
 * which carries the language as its extension. The translator takes an output
 * file; it is given /dev/stdout so the result is captured directly.
 *
 * A case/X.args is a run that must fail: the translator is given the
 * arguments in it, and standard input from case/X.in if there is one, and
 * must exit with 1 and print case/X.err on standard error.
 */

#include "harness.h"

/* The field between the first and second '-': test-go-02.ri is go. */
static std::string cgilLang( const std::string &file )
{
	size_t first = file.find( '-' );
	if ( first == std::string::npos )
		return std::string();
	size_t second = file.find( '-', first + 1 );
	if ( second == std::string::npos )
		return std::string();
	return file.substr( first + 1, second - first - 1 );
}

/* A case/X.args run: the arguments in it, standard input from case/X.in if
 * there is one, exit 1 and case/X.err on standard error. */
static void failingRun( const Config &config, Job *job, const std::string &caseDir,
		const std::string &build, const std::string &root, const std::string &lang )
{
	std::string args, expected;
	if ( !readFile( joinPath( caseDir, root + ".args" ), args ) ) {
		job->error( "cannot read " + root + ".args" );
		return;
	}
	if ( !readFile( joinPath( caseDir, root + ".err" ), expected ) ) {
		job->error( "cannot read " + root + ".err" );
		return;
	}

	Words argv;
	argv.push_back( joinPath( config.cgilDir, "cgil-" + lang ) );
	Words words = splitWords( args );
	argv.insert( argv.end(), words.begin(), words.end() );

	Step run = Step::exec( Step::Run, argv, build ).exit( 1 ).stderrOutput();
	std::string in = joinPath( caseDir, root + ".in" );
	if ( fileExists( in ) )
		run.stdinFrom( in );
	job->steps.push_back( run );
	job->steps.push_back( Step::compare( expected, "" ) );
}

void enumerateCgil( const Config &config, const Selection &sel, JobList &jobs )
{
	const char *suite = "cgil.d";
	std::string src = config.suiteSrc( suite );
	std::string build = config.suiteBuild( suite );
	std::string wk = config.working( suite );
	std::string caseDir = joinPath( src, "case" );

	std::vector<std::string> names;
	if ( !listCaseDir( suite, caseDir, names, jobs ) )
		return;

	for ( size_t i = 0; i < names.size(); i++ ) {
		bool fails = hasSuffix( names[i], ".args" );
		if ( !fails && !hasSuffix( names[i], ".ri" ) )
			continue;
		std::string root = stripSuffix( names[i], fails ? ".args" : ".ri" );
		if ( !sel.selects( names[i] ) && !sel.selects( "case/" + names[i] ) )
			continue;

		Job *job = new Job( suite, root );
		job->reportPath = joinPath( wk, root + ".diff" );
		jobs.append( job );

		std::string lang = cgilLang( names[i] );
		if ( lang.empty() ) {
			job->error( "no language in file name" );
			continue;
		}

		if ( fails ) {
			failingRun( config, job, caseDir, build, root, lang );
			continue;
		}

		std::string expected;
		if ( !readFile( joinPath( caseDir, root + "." + lang ), expected ) ) {
			job->error( "cannot read " + root + "." + lang );
			continue;
		}

		Words argv;
		argv.push_back( joinPath( config.cgilDir, "cgil-" + lang ) );
		argv.push_back( joinPath( caseDir, names[i] ) );
		argv.push_back( "/dev/stdout" );
		/* A translator that fails must say so in its exit status. The old
		 * runtests ran under set -e and relied on that. */
		job->steps.push_back( Step::exec( Step::Run, argv, build )
				.capture( CaptureOutput ).exit( 0 ) );
		job->steps.push_back( Step::compare( expected, "" ) );
	}
}
