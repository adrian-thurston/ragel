/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

#include "harness.h"

#include <glob.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

Step Step::exec( Role role, const Words &argv, const std::string &cwd )
{
	Step s( Exec );
	s.role = role;
	s.argv = argv;
	s.cwd = cwd;
	s.expectExit = 0;
	return s;
}

Step Step::filter( const std::string &shell, const std::string &cwd )
{
	Step s( Filter );
	s.shell = shell;
	s.cwd = cwd;
	return s;
}

Step Step::writeFile( const std::string &path, const std::string &content )
{
	Step s( WriteFile );
	s.path = path;
	s.content = content;
	return s;
}

Step Step::editFile( const std::string &path, const std::string &from, const std::string &to )
{
	Step s( EditFile );
	s.path = path;
	s.from = from;
	s.to = to;
	return s;
}

Step Step::compare( const std::string &expected, const std::string &label )
{
	Step s( Compare );
	s.expected = expected;
	s.label = label;
	return s;
}

void Job::fail( const std::string &why )
{
	if ( outcome == Pending || outcome == Pass )
		outcome = Fail;
	if ( !reason.empty() )
		reason += "; ";
	reason += why;
}

static const char *roleName( Step::Role role )
{
	switch ( role ) {
		case Step::Prepare: return "preparation";
		case Step::Compile: return "compilation";
		case Step::Run: return "run";
	}
	return "step";
}

static bool isWordChar( char c )
{
	return isalnum( (unsigned char)c ) || c == '_';
}

/* Replace whole-word occurrences, as sed 's/\<from\>/to/g'. */
static void replaceWords( std::string &s, const std::string &from, const std::string &to )
{
	if ( from.empty() )
		return;
	std::string out;
	size_t pos = 0;
	while ( pos < s.size() ) {
		size_t at = s.find( from, pos );
		if ( at == std::string::npos ) {
			out.append( s, pos, std::string::npos );
			break;
		}
		size_t end = at + from.size();
		bool startOk = at == 0 || !isWordChar( s[at-1] );
		bool endOk = end >= s.size() || !isWordChar( s[end] );
		if ( startOk && endOk ) {
			out.append( s, pos, at - pos );
			out += to;
			pos = end;
		}
		else {
			out.append( s, pos, at + 1 - pos );
			pos = at + 1;
		}
	}
	s = out;
}

static std::string commandLine( const Step &step )
{
	if ( step.kind == Step::Filter )
		return "$ ... | " + step.shell + "\n";
	if ( step.kind != Step::Exec )
		return std::string();
	std::string line = "$ ";
	if ( !step.cwd.empty() )
		line += "cd " + shellQuote( step.cwd ) + " && ";
	for ( size_t i = 0; i < step.env.size(); i++ ) {
		size_t eq = step.env[i].find( '=' );
		line += step.env[i].substr( 0, eq + 1 ) +
				shellQuote( step.env[i].substr( eq + 1 ) ) + " ";
	}
	for ( size_t i = 0; i < step.argv.size(); i++ ) {
		if ( i > 0 )
			line += " ";
		line += shellQuote( step.argv[i] );
	}
	if ( !step.stdinFile.empty() )
		line += " < " + shellQuote( step.stdinFile );
	return line + "\n";
}

/* One line per step, for --commands. */
std::string describeStep( const Step &step )
{
	switch ( step.kind ) {
		case Step::Exec:
		case Step::Filter:
			return commandLine( step );
		case Step::WriteFile:
			return "write " + step.path + "\n";
		case Step::EditFile:
			return "edit " + step.path + ": " + step.from + " -> " + step.to + "\n";
		case Step::Compare:
			return std::string( "compare" ) + ( step.ignoreWs ? " -w" : "" ) +
					( step.stripCr ? " --strip-trailing-cr" : "" ) +
					( step.label.empty() ? "" : " " + step.label ) + "\n";
	}
	return std::string();
}

static void splitLines( const std::string &text, std::vector<std::string> &lines )
{
	size_t pos = 0;
	while ( pos < text.size() ) {
		size_t eol = text.find( '\n', pos );
		if ( eol == std::string::npos )
			eol = text.size();
		lines.push_back( text.substr( pos, eol - pos ) );
		pos = eol + 1;
	}
}

/* Fail the job for each line of an Exec's standard error that starts with
 * its error prefix, giving the rest of the line as the reason, unless the rest
 * is one of its known errors. */
static void failOnErrors( Job &job, const Step &step, const std::string &errOut,
		const std::string &run )
{
	const std::string &prefix = step.errorPrefix;
	std::vector<std::string> lines, known;
	splitLines( errOut, lines );
	splitLines( step.knownErrors, known );
	for ( size_t i = 0; i < known.size(); i++ )
		known[i] = trim( known[i] );

	for ( size_t i = 0; i < lines.size(); i++ ) {
		if ( lines[i].compare( 0, prefix.size(), prefix ) != 0 )
			continue;
		std::string rest = trim( lines[i].substr( prefix.size() ) );
		if ( std::find( known.begin(), known.end(), rest ) == known.end() )
			job.fail( run + rest );
	}
}

static void runJob( const Config &config, Job &job )
{
	for ( size_t si = 0; si < job.steps.size(); si++ ) {
		const Step &step = job.steps[si];
		switch ( step.kind ) {
			case Step::WriteFile: {
				if ( !writeFile( step.path, step.content ) ) {
					job.error( "cannot write " + step.path );
					return;
				}
				break;
			}
			case Step::EditFile: {
				std::string content;
				if ( !readFile( step.path, content ) ) {
					job.fail( "no file to edit: " + step.path );
					return;
				}
				replaceWords( content, step.from, step.to );
				if ( !writeFile( step.path, content ) ) {
					job.error( "cannot write " + step.path );
					return;
				}
				break;
			}
			case Step::Exec: {
				job.log += commandLine( step );

				std::string *outBuf = 0;
				if ( step.stdoutTo == CaptureOutput ) {
					job.output.clear();
					outBuf = &job.output;
				}
				else if ( step.stdoutTo == CaptureLog ) {
					outBuf = &job.log;
				}

				/* Standard error goes to the log, by way of a buffer of its own
				 * when it is to be searched or becomes the output. */
				std::string errOut;
				std::string *errBuf = step.errorPrefix.empty() && !step.stderrIsOutput ?
						&job.log : &errOut;

				int exitCode = 0;
				std::string err;
				if ( !runProcess( step.argv, step.cwd, step.env, 0, step.stdinFile,
						outBuf, errBuf, exitCode, err ) )
				{
					job.error( "cannot run " + err );
					return;
				}
				job.exitCode = exitCode;
				job.log += errOut;
				if ( step.stderrIsOutput )
					job.output = errOut;

				std::string run = step.label.empty() ? "" : step.label + ": ";

				if ( !step.errorPrefix.empty() )
					failOnErrors( job, step, errOut, run );

				if ( step.errorExit >= 0 && exitCode == step.errorExit ) {
					job.fail( run + baseName( step.argv[0] ) + " found errors" );
					break;
				}

				if ( step.expectExit >= 0 && exitCode != step.expectExit ) {
					char buf[64];
					snprintf( buf, sizeof(buf), "%d", exitCode );
					job.log += std::string( "[exit " ) + buf + "]\n";
					if ( step.role == Step::Run ) {
						char msg[128];
						snprintf( msg, sizeof(msg), "exit value: got %d expected %d",
								exitCode, step.expectExit );
						job.fail( run + msg );
					}
					else if ( step.expectExit != 0 ) {
						/* A step expected to fail, such as a compilation
						 * expected to give an error. */
						char msg[128];
						snprintf( msg, sizeof(msg), "%s exit value: got %d expected %d",
								roleName( step.role ), exitCode, step.expectExit );
						job.fail( msg );
						return;
					}
					else {
						job.fail( std::string( roleName( step.role ) ) + " failed" );
						return;
					}
				}
				break;
			}
			case Step::Filter: {
				job.log += "$ ... | " + step.shell + "\n";
				Words argv;
				argv.push_back( "/bin/sh" );
				argv.push_back( "-c" );
				argv.push_back( step.shell );
				std::string filtered;
				int exitCode = 0;
				std::string err;
				if ( !runProcess( argv, step.cwd, Words(), &job.output, "",
						&filtered, &job.log, exitCode, err ) )
				{
					job.error( "cannot run filter: " + err );
					return;
				}
				if ( exitCode != 0 ) {
					job.fail( "filter failed" );
					return;
				}
				job.output = filtered;
				break;
			}
			case Step::Compare: {
				if ( !outputsMatch( step.expected, job.output, step.ignoreWs, step.stripCr ) ) {
					job.fail( step.label.empty() ? "output differs" : step.label + " differs" );
					job.diff += unifiedDiff( step.expected, job.output,
							step.ignoreWs, step.stripCr,
							step.label.empty() ? "expected" : step.label + " expected",
							step.label.empty() ? "actual" : step.label + " actual" );
				}
				break;
			}
		}
	}

	if ( job.outcome == Pending )
		job.outcome = Pass;
}

static void removeArtifacts( const Job &job )
{
	for ( size_t i = 0; i < job.artifacts.size(); i++ ) {
		glob_t g;
		memset( &g, 0, sizeof(g) );
		if ( glob( job.artifacts[i].c_str(), 0, 0, &g ) == 0 ) {
			for ( size_t k = 0; k < g.gl_pathc; k++ ) {
				if ( unlink( g.gl_pathv[k] ) != 0 )
					rmdir( g.gl_pathv[k] );
			}
		}
		globfree( &g );
	}
}

static void writeReport( const Job &job )
{
	if ( job.reportPath.empty() )
		return;
	std::string report;
	report += job.id() + ": " + job.reason + "\n";
	if ( !job.diff.empty() )
		report += "\n" + job.diff;
	if ( !job.log.empty() )
		report += "\n---- log\n" + job.log;
	writeFile( job.reportPath, report );
}

struct Scheduler
{
	Scheduler( const Config &config, JobList &jobs )
		: config(config), jobs(jobs), remaining(0) {}

	const Config &config;
	JobList &jobs;

	std::mutex lock;
	std::condition_variable wake;
	std::priority_queue< int, std::vector<int>, std::greater<int> > ready;
	int remaining;

	std::mutex outputLock;

	void print( const Job &job );
	void complete( int idx );
	void worker();
	void run();
};

void Scheduler::print( const Job &job )
{
	if ( !job.counted && job.outcome == Pass )
		return;

	const char *tag = "";
	switch ( job.outcome ) {
		case Pass: tag = "PASS"; break;
		case Fail: tag = "FAIL"; break;
		case Skip: tag = "SKIP"; break;
		case Error: tag = "ERROR"; break;
		case Pending: tag = "PENDING"; break;
	}

	if ( job.outcome == Pass && !config.verbose )
		return;
	if ( job.outcome == Skip && !config.verbose )
		return;

	std::lock_guard<std::mutex> guard( outputLock );
	std::string line = std::string( tag ) + " " + job.id();
	if ( !job.reason.empty() )
		line += ": " + job.reason;
	if ( job.outcome == Fail && !job.reportPath.empty() ) {
		std::string path = job.reportPath;
		std::string prefix = config.builddir + "/";
		if ( path.compare( 0, prefix.size(), prefix ) == 0 )
			path = path.substr( prefix.size() );
		line += " (" + path + ")";
	}
	printf( "%s\n", line.c_str() );
	fflush( stdout );
}

/* Called with the lock held. */
void Scheduler::complete( int idx )
{
	Job &job = *jobs[idx];
	remaining--;

	for ( size_t d = 0; d < job.dependents.size(); d++ ) {
		int di = job.dependents[d];
		Job &dep = *jobs[di];
		if ( --dep.pending == 0 ) {
			if ( job.outcome == Pass ) {
				ready.push( di );
			}
			else {
				const char *what = job.outcome == Skip ? "skipped" :
						job.outcome == Error ? "could not run" : "failed";
				if ( job.outcome == Skip )
					dep.skip( job.name + " " + what );
				else
					dep.fail( job.name + " " + what );
				print( dep );
				complete( di );
			}
		}
	}
	wake.notify_all();
}

void Scheduler::worker()
{
	while ( true ) {
		int idx;
		{
			std::unique_lock<std::mutex> guard( lock );
			while ( ready.empty() && remaining > 0 )
				wake.wait( guard );
			if ( ready.empty() )
				return;
			idx = ready.top();
			ready.pop();
		}

		Job &job = *jobs[idx];
		runJob( config, job );
		if ( job.outcome == Pass ) {
			if ( !config.keep )
				removeArtifacts( job );
		}
		else {
			writeReport( job );
		}
		print( job );

		std::lock_guard<std::mutex> guard( lock );
		complete( idx );
	}
}

void Scheduler::run()
{
	/* Dependency counts, then seed the ready queue. */
	for ( long i = 0; i < jobs.length(); i++ ) {
		Job &job = *jobs[i];
		job.pending = job.deps.size();
		for ( size_t d = 0; d < job.deps.size(); d++ )
			jobs[job.deps[d]]->dependents.push_back( i );
	}

	{
		std::lock_guard<std::mutex> guard( lock );
		for ( long i = 0; i < jobs.length(); i++ ) {
			if ( jobs[i]->outcome == Pending )
				remaining++;
		}
		for ( long i = 0; i < jobs.length(); i++ ) {
			Job &job = *jobs[i];
			if ( job.outcome != Pending ) {
				/* Resolved at enumeration: skipped or in error. */
				print( job );
				remaining++;
				complete( i );
			}
			else if ( job.pending == 0 ) {
				ready.push( i );
			}
		}
	}

	int n = config.jobs > 0 ? config.jobs : 1;
	std::vector<std::thread> threads;
	for ( int i = 0; i < n; i++ )
		threads.push_back( std::thread( &Scheduler::worker, this ) );
	for ( int i = 0; i < n; i++ )
		threads[i].join();
}

void runJobs( const Config &config, JobList &jobs )
{
	Scheduler scheduler( config, jobs );
	scheduler.run();
}
