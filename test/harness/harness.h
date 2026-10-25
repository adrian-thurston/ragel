/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * The test harness. Every suite is enumerated into the same description: a
 * Job is a named sequence of Steps, optionally depending on other jobs, and
 * every Step is one of a small set of operations (run a program, filter the
 * captured output through a shell command, write or edit a file, compare the
 * captured output against expected text). The scheduler then runs all jobs
 * from all suites through one work queue.
 */

#ifndef _HARNESS_H
#define _HARNESS_H

#include <string>
#include <vector>
#include <set>

#include "vector.h"

typedef std::vector<std::string> Words;

/*
 * Configuration: build-time values from config.h, adjusted by command line
 * options.
 */
struct Config
{
	/* The test source and build directories (the test/ directories). */
	std::string srcdir;
	std::string builddir;

	/* The top of the source tree, which holds the manual's examples, and of
	 * the build tree. */
	std::string topSrcdir;
	std::string topBuilddir;

	std::string cc, cxx;

	/* Colm under test. */
	std::string colmBin;
	Words colmCppflags;
	Words colmLdflags;

	/* The directory holding the cgil translators under test. */
	std::string cgilDir;

	/* Ragel under test: the main binary and the host-language frontends. */
	std::string ragelBin;
	std::string ragelC, ragelD, ragelJava, ragelRuby, ragelCsharp, ragelGo,
			ragelOcaml, ragelAsm, ragelRust, ragelZig, ragelCrack, ragelJulia,
			ragelJs;

	/* Host language toolchains. Empty when not found by configure. */
	std::string dBin, javacBin, rubyBin, csharpBin, goBin, ocamlBin, rustBin,
			zigBin, crackBin, juliaBin, nodeBin, asmBin, gnustepConfig;

	/* Options. */
	int jobs;
	bool verbose;
	bool keep;
	bool list;
	bool commands;
	std::string tapFile;
	std::vector<std::string> filters;

	/* ragel.d selection. Empty means the default set. */
	std::set<std::string> langs;
	std::vector<std::string> genflags;

	/* aapl.d: how long each stress program runs. Zero skips them. */
	int stressSecs;

	/* colm.d and manual: run the programs under valgrind. */
	bool valgrind;

	Config()
		: jobs(0), verbose(false), keep(false), list(false), commands(false),
		stressSecs(5), valgrind(false) {}

	std::string suiteSrc( const char *suite ) const;
	std::string suiteBuild( const char *suite ) const;
	std::string working( const char *suite ) const;
};

/* A selection of cases, by case file basename. Empty selects everything. */
struct Selection
{
	std::set<std::string> files;
	bool selects( const std::string &path ) const;
};

/* Where a program's standard output goes. */
enum Capture
{
	CaptureNone,     /* discarded */
	CaptureOutput,   /* becomes the job's current output, to be compared */
	CaptureLog       /* appended to the job log, with stderr */
};

struct Step
{
	enum Kind { Exec, Filter, WriteFile, EditFile, Compare };

	/* How the failure of an Exec is reported. */
	enum Role { Prepare, Compile, Run };

	Kind kind;
	Role role;

	/* Exec. */
	Words argv;
	std::string cwd;
	Words env;               /* NAME=value, added to the environment */
	std::string stdinFile;
	Capture stdoutTo;
	int expectExit;          /* -1: not checked */

	/* Exec: errors found in the program, which fail the case: errorExit, the
	 * exit value of a checker it runs under (valgrind) that found some, and
	 * standard error lines that start with errorPrefix, unless the rest of the
	 * line is one of the lines of knownErrors. -1 and empty: not checked. */
	int errorExit;
	std::string errorPrefix;
	std::string knownErrors;

	/* Exec: standard error becomes the job's current output, as well as going
	 * to the log. Standard output must then not be CaptureOutput. */
	bool stderrIsOutput;

	/* Filter: a shell command the current output is piped through. */
	std::string shell;

	/* WriteFile: content to path. EditFile: replace whole-word from with to
	 * in path. */
	std::string path;
	std::string content;
	std::string from, to;

	/* Compare: the current output against expected. */
	std::string expected;
	bool ignoreWs;
	bool stripCr;

	/* Compare and Exec: names the run in failure reasons. */
	std::string label;

	Step( Kind kind )
		: kind(kind), role(Run), stdoutTo(CaptureNone),
		expectExit(-1), errorExit(-1), stderrIsOutput(false),
		ignoreWs(false), stripCr(false) {}

	static Step exec( Role role, const Words &argv, const std::string &cwd );
	static Step filter( const std::string &shell, const std::string &cwd );
	static Step writeFile( const std::string &path, const std::string &content );
	static Step editFile( const std::string &path,
			const std::string &from, const std::string &to );
	static Step compare( const std::string &expected, const std::string &label );

	Step &stdinFrom( const std::string &file ) { stdinFile = file; return *this; }
	Step &capture( Capture c ) { stdoutTo = c; return *this; }
	Step &exit( int e ) { expectExit = e; return *this; }
	Step &environment( const std::string &setting ) { env.push_back( setting ); return *this; }
	Step &errorsFrom( int e, const std::string &prefix, const std::string &known )
		{ errorExit = e; errorPrefix = prefix; knownErrors = known; return *this; }
	Step &stderrOutput() { stderrIsOutput = true; return *this; }
	Step &labelled( const std::string &l ) { label = l; return *this; }
	Step &whitespace() { ignoreWs = true; return *this; }
	Step &trailingCr() { stripCr = true; return *this; }
};

enum Outcome
{
	Pending,
	Pass,
	Fail,     /* the subject under test misbehaved */
	Skip,     /* not run: no toolchain, disabled */
	Error     /* the harness could not run the case */
};

struct Job
{
	std::string suite;
	std::string name;
	std::vector<Step> steps;

	/* Indices of jobs that must pass before this one runs. */
	std::vector<int> deps;

	/* Files, and the directories they empty, to remove after a pass, as glob
	 * patterns. */
	std::vector<std::string> artifacts;

	/* False for shared preparation work that is not itself a case. */
	bool counted;

	/* Where the failure report goes. */
	std::string reportPath;

	/* Result. */
	Outcome outcome;
	std::string reason;
	std::string log;
	std::string diff;
	std::string output;
	int exitCode;

	/* Scheduling state. */
	int pending;
	std::vector<int> dependents;

	Job( const std::string &suite, const std::string &name )
		: suite(suite), name(name), counted(true), outcome(Pending),
		exitCode(0), pending(0) {}

	std::string id() const { return suite + "/" + name; }
	void skip( const std::string &why ) { outcome = Skip; reason = why; }
	void error( const std::string &why ) { outcome = Error; reason = why; }
	void fail( const std::string &why );
};

typedef Vector<Job*> JobList;

/*
 * Suites.
 */
struct Suite
{
	const char *name;
	void (*enumerate)( const Config &config, const Selection &sel, JobList &jobs );
};

extern Suite suites[];
extern const int numSuites;

void enumerateAapl( const Config &config, const Selection &sel, JobList &jobs );
void enumerateCgil( const Config &config, const Selection &sel, JobList &jobs );
void enumerateRlhc( const Config &config, const Selection &sel, JobList &jobs );
void enumerateRlparse( const Config &config, const Selection &sel, JobList &jobs );
void enumerateTrans( const Config &config, const Selection &sel, JobList &jobs );
void enumerateColm( const Config &config, const Selection &sel, JobList &jobs );
void enumerateManual( const Config &config, const Selection &sel, JobList &jobs );
void enumerateRagel( const Config &config, const Selection &sel, JobList &jobs );

/*
 * Colm programs, as colm.d and the manual compile and run them.
 */

/* The program, its compilation arguments, and the C functions it calls (CALL)
 * and the host program it is linked into (HOST), if it has them, and the
 * reducer.h its commit code includes (reducer, colm.d's REDUCER or the
 * manual's NAME.reducer.h). A program with compErr (COMP_ERR) is expected to
 * fail to compile, with that error. A C host (hostIsC, the manual's
 * NAME.host.c) gets no export header or code. */
struct ColmProgram
{
	std::string text;
	Words comp;
	bool hasCall, hasHost, hostIsC, hasReducer, hasCompErr;
	std::string call, host, reducer, compErr;

	ColmProgram()
		: hasCall(false), hasHost(false), hostIsC(false), hasReducer(false),
		hasCompErr(false) {}
};

/* One run of the compiled program. */
struct ColmRun
{
	Words args;
	std::string stdinFile;   /* empty: no input */
	int exitValue;
	std::string lost;        /* the leak reports that don't fail the run */
	std::string expected;
	std::string label;

	ColmRun() : exitValue(0) {}
};

/* colmCompile adds the steps that compile the program to working/NAME in the
 * job's suite build directory, NAME being the job's name, or, for a program
 * with compErr, that check colm's error. colmRun adds a run of it, checked
 * for its output, its exit value and its leak reports. */
void colmCompile( const Config &config, Job *job, const ColmProgram &prog );
void colmRun( const Config &config, Job *job, const ColmRun &run );

/*
 * Case files: the common section and directive language.
 *
 * A section header is a line of the form "#### NAME ####". A directive is a
 * line containing "@NAME: value". The text before the first header is the
 * preamble.
 */
struct Section
{
	std::string name;
	std::string body;
	long headerStart;   /* offset of the header line */
	long bodyStart;     /* offset of the line after it */
};

struct CaseFile
{
	std::string path;
	std::string text;
	std::string preamble;
	std::vector<Section> sections;
	std::vector< std::pair<std::string, std::string> > directives;

	/* Anchored headers must start the line, as in ragel.d. Otherwise a header
	 * may appear anywhere in a line and the name is the line with every '#'
	 * and ' ' removed, as in colm.d. */
	bool load( const std::string &path, bool anchored, std::string &err );

	const Section *find( const char *name, int nth = 0 ) const;
	bool has( const char *name ) const { return find( name ) != 0; }

	/* All values of a directive, one per line, as the shell drivers' sed
	 * extraction yields. Empty if absent. */
	std::string directive( const char *key ) const;

	/* The text after a section's header line, to the end of the file, and the
	 * text before the header line. */
	std::string after( const Section &s ) const;
	std::string before( const Section &s ) const;
};

/*
 * Processes.
 */

/* Run argv in cwd, with the NAME=value settings in env added to the
 * environment. Standard input comes from stdinData if non-null, else from
 * stdinFile if non-empty, else /dev/null. Standard output is appended to
 * stdoutBuf if non-null, else discarded. Standard error is appended to
 * stderrBuf if non-null, else inherited. Returns false if the process could
 * not be started, with errMsg set. The exit code is the exit status, or 128
 * plus the signal number. */
bool runProcess( const Words &argv, const std::string &cwd, const Words &env,
		const std::string *stdinData, const std::string &stdinFile,
		std::string *stdoutBuf, std::string *stderrBuf,
		int &exitCode, std::string &errMsg );

/*
 * Comparison.
 */
bool outputsMatch( const std::string &expected, const std::string &actual,
		bool ignoreWs, bool stripCr );
std::string unifiedDiff( const std::string &expected, const std::string &actual,
		bool ignoreWs, bool stripCr,
		const std::string &expLabel, const std::string &actLabel );

/*
 * Execution.
 */
void runJobs( const Config &config, JobList &jobs );
std::string describeStep( const Step &step );

/*
 * Utilities.
 */
std::string joinPath( const std::string &dir, const std::string &file );
std::string baseName( const std::string &path );
std::string stripSuffix( const std::string &name, const char *suffix );
bool hasSuffix( const std::string &name, const char *suffix );
bool readFile( const std::string &path, std::string &out );
bool writeFile( const std::string &path, const std::string &content );
bool fileExists( const std::string &path );
bool isDir( const std::string &path );
bool mkdirp( const std::string &path );
void clearDir( const std::string &path );
bool listDir( const std::string &path, std::vector<std::string> &names );
bool listCaseDir( const char *suite, const std::string &path,
		std::vector<std::string> &names, JobList &jobs );
Words splitWords( const std::string &s );
std::string joinWords( const Words &w );
std::string trim( const std::string &s );
bool wordIn( const std::string &word, const std::string &list );
void replaceAll( std::string &s, const std::string &from, const std::string &to );
std::string shellQuote( const std::string &s );

#endif
