/*
 *  Copyright 2001-2005 Adrian Thurston <thurston@cs.queensu.ca>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>

#include "rlcodegen.h"
#include "rlcodegen.h"
#include "xmlparse.h"
#include "pcheck.h"
#include "vector.h"
#include "version.h"

#include "common.cpp"

using std::istream;
using std::ifstream;
using std::ostream;
using std::ios;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

/* Target language and output style. */
OutputFormat outputFormat = OutCode;
CodeStyleEnum codeStyle = GenTables;

/* Io globals. */
istream *inStream = 0;
ostream *outStream = 0;
output_filter *outFilter = 0;
char *outputFileName = 0;

/* Graphviz dot file generation. */
bool graphvizDone = false;

int numSplitPartitions = 0;
bool printPrintables = false;

/* Print a summary of the options. */
void usage()
{
	cout <<
"usage: rlcodegen [options] file\n"
"general:\n"
"   -h, -H, -?, --help    Print this usage and exit\n"
"   -v, --version         Print version information and exit\n"
"   -o <file>             Write output to <file>\n"
"output:\n"
"   -V                    Generate a Graphviz dotfile instead of code\n"
"   -p                    Print printable characters in Graphviz output\n"
"generated code style:\n"
"   -T0                   Table driven FSM (default)\n"
"   -T1                   Faster table driven FSM\n"
"   -F0                   Flat table driven FSM\n"
"   -F1                   Faster flat table-driven FSM\n"
"   -G0                   Goto-driven FSM\n"
"   -G1                   Faster goto-driven FSM\n"
"   -G2                   Really fast goto-driven FSM\n"
"   -P<N>                 N-Way Split really fast goto-driven FSM\n"
	;	
}

/* Print version information. */
void version()
{
	cout << "Ragel Code Generator version " VERSION << " " PUBDATE << endl <<
			"Copyright (c) 2001-2006 by Adrian Thurston" << endl;
}

/* Scans a string looking for the file extension. If there is a file
 * extension then pointer returned points to inside the string
 * passed in. Otherwise returns null. */
char *findFileExtension( char *stemFile )
{
	char *ppos = stemFile + strlen(stemFile) - 1;

	/* Scan backwards from the end looking for the first dot.
	 * If we encounter a '/' before the first dot, then stop the scan. */
	while ( 1 ) {
		/* If we found a dot or got to the beginning of the string then
		 * we are done. */
		if ( ppos == stemFile || *ppos == '.' )
			break;

		/* If we hit a / then there is no extension. Done. */
		if ( *ppos == '/' ) {
			ppos = stemFile;
			break;
		}
		ppos--;
	} 

	/* If we got to the front of the string then bail we 
	 * did not find an extension  */
	if ( ppos == stemFile )
		ppos = 0;

	return ppos;
}

/* Make a file name from a stem. Removes the old filename suffix and
 * replaces it with a new one. Returns a newed up string. */
char *fileNameFromStem( char *stemFile, char *suffix )
{
	int len = strlen( stemFile );
	assert( len > 0 );

	/* Get the extension. */
	char *ppos = findFileExtension( stemFile );

	/* If an extension was found, then shorten what we think the len is. */
	if ( ppos != 0 )
		len = ppos - stemFile;

	/* Make the return string from the stem and the suffix. */
	char *retVal = new char[ len + strlen( suffix ) + 1 ];
	strncpy( retVal, stemFile, len );
	strcpy( retVal + len, suffix );

	return retVal;
}

/* Total error count. */
int gblErrorCount = 0;

ostream &error()
{
	gblErrorCount += 1;
	cerr << PROGNAME ": ";
	return cerr;
}

/* Counts newlines before sending sync. */
int output_filter::sync( )
{
	line += 1;
	return std::filebuf::sync();
}

/* Counts newlines before sending data out to file. */
std::streamsize output_filter::xsputn( const char *s, std::streamsize n )
{
	for ( int i = 0; i < n; i++ ) {
		if ( s[i] == '\n' )
			line += 1;
	}
	return std::filebuf::xsputn( s, n );
}

void escapeLineDirectivePath( std::ostream &out, char *path )
{
	for ( char *pc = path; *pc != 0; pc++ ) {
		if ( *pc == '\\' )
			out << "\\\\";
		else
			out << *pc;
	}
}

/* Invoked by the parser, after the source file 
 * name is taken from XML file. */
void openOutput( char *inputFile )
{
	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFormat == OutCode && outputFileName == 0 ) {
		char *ext = findFileExtension( inputFile );
		if ( ext != 0 && strcmp( ext, ".rh" ) == 0 )
			outputFileName = fileNameFromStem( inputFile, ".h" );
		else {
			char *defExtension = 0;
			switch ( hostLangType ) {
				case CCode: defExtension = ".c"; break;
				case DCode: defExtension = ".d"; break;
				case JavaCode: defExtension = ".java"; break;
			}
			outputFileName = fileNameFromStem( inputFile, defExtension );
		}
	}

	/* Make sure we are not writing to the same file as the input file. */
	if ( outputFileName != 0 && strcmp( inputFile, outputFileName  ) == 0 ) {
		error() << "output file \"" << outputFileName  << 
				"\" is the same as the input file" << endl;
	}

	if ( outputFileName != 0 ) {
		/* Create the filter on the output and open it. */
		outFilter = new output_filter;
		outFilter->open( outputFileName, ios::out|ios::trunc );
		if ( !outFilter->is_open() ) {
			error() << "error opening " << outputFileName << " for writing" << endl;
			exit(1);
		}

		/* Open the output stream, attaching it to the filter. */
		outStream = new ostream( outFilter );
	}
	else {
		/* Writing out ot std out. */
		outStream = &cout;
	}
}

/* Main, process args and call yyparse to start scanning input. */
int main(int argc, char **argv)
{
	ParamCheck pc("o:VpT:F:G:vHh?-:P:", argc, argv);
	char *xmlInputFileName = 0;

	while ( pc.check() ) {
		switch ( pc.state ) {
		case ParamCheck::match:
			switch ( pc.parameter ) {
			/* Output. */
			case 'o':
				if ( *pc.parameterArg == 0 )
					error() << "a zero length output file name was given" << endl;
				else if ( outputFileName != 0 )
					error() << "more than one output file name was given" << endl;
				else {
					/* Ok, remember the output file name. */
					outputFileName = pc.parameterArg;
				}
				break;

			/* Output formats. */
			case 'V':
				outputFormat = OutGraphvizDot;
				break;

			case 'p':
				printPrintables = true;
				break;

			/* Code style. */
			case 'T':
				if ( pc.parameterArg[0] == '0' )
					codeStyle = GenTables;
				else if ( pc.parameterArg[0] == '1' )
					codeStyle = GenFTables;
				else {
					error() << "-T" << pc.parameterArg[0] << 
							" is an invalid argument" << endl;
					exit(1);
				}
				break;
			case 'F':
				if ( pc.parameterArg[0] == '0' )
					codeStyle = GenFlat;
				else if ( pc.parameterArg[0] == '1' )
					codeStyle = GenFFlat;
				else {
					error() << "-F" << pc.parameterArg[0] << 
							" is an invalid argument" << endl;
					exit(1);
				}
				break;
			case 'G':
				if ( pc.parameterArg[0] == '0' )
					codeStyle = GenGoto;
				else if ( pc.parameterArg[0] == '1' )
					codeStyle = GenFGoto;
				else if ( pc.parameterArg[0] == '2' )
					codeStyle = GenIpGoto;
				else {
					error() << "-G" << pc.parameterArg[0] << 
							" is an invalid argument" << endl;
					exit(1);
				}
				break;
			case 'P':
				codeStyle = GenSplit;
				numSplitPartitions = atoi( pc.parameterArg );
				break;

			/* Version and help. */
			case 'v':
				version();
				exit(0);
			case 'H': case 'h': case '?':
				usage();
				exit(0);
			case '-':
				if ( strcasecmp(pc.parameterArg, "help") == 0 ) {
					usage();
					exit(0);
				}
				else if ( strcasecmp(pc.parameterArg, "version") == 0 ) {
					version();
					exit(0);
				}
				else {
					error() << "--" << pc.parameterArg << 
							" is an invalid argument" << endl;
					break;
				}
			}
			break;

		case ParamCheck::invalid:
			error() << "-" << pc.parameter << " is an invalid argument" << endl;
			break;

		case ParamCheck::noparam:
			if ( *pc.curArg == 0 )
				error() << "a zero length input file name was given" << endl;
			else if ( xmlInputFileName != 0 )
				error() << "more than one input file name was given" << endl;
			else {
				/* OK, Remember the filename. */
				xmlInputFileName = pc.curArg;
			}
			break;
		}
	}

	/* Bail on above errors. */
	if ( gblErrorCount > 0 )
		exit(1);

	/* Open the input file for reading. */
	if ( xmlInputFileName != 0 ) {
		/* Open the input file for reading. */
		ifstream *inFile = new ifstream( xmlInputFileName );
		inStream = inFile;
		if ( ! inFile->is_open() )
			error() << "could not open " << xmlInputFileName << " for reading" << endl;
	}
	else {
		xmlInputFileName = "<stdin>";
		inStream = &cin;
	}

	/* Bail on above errors. */
	if ( gblErrorCount > 0 )
		exit(1);

	/* Parse the input! */
	xml_parse( *inStream, xmlInputFileName );

	/* If writing to a file, delete the ostream, causing it to flush.
	 * Standard out is flushed automatically. */
	if ( outputFileName != 0 ) {
		delete outStream;
		delete outFilter;
	}

	/* Finished, final check for errors.. */
	if ( gblErrorCount > 0 ) {
		/* If we opened an output file, remove it. */
		if ( outputFileName != 0 )
			unlink( outputFileName );
		exit(1);
	}
	return 0;
}
