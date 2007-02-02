/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@cs.queensu.ca>
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

#ifndef _RLGEN_JAVA_H
#define _RLGEN_JAVA_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include "avltree.h"
#include "vector.h"
#include "config.h"

#define PROGNAME "rlcodegen"

/* Target language. */
enum OutputFormat
{
	OutCode,
	OutGraphvizDot
};

/* Target output style. */
enum CodeStyleEnum
{
	GenTables,
	GenFTables,
	GenFlat,
	GenFFlat,
	GenGoto,
	GenFGoto,
	GenIpGoto,
	GenSplit
};

/* Filter on the output stream that keeps track of the number of lines
 * output. */
class output_filter : public std::filebuf
{
public:
	output_filter( char *fileName ) : fileName(fileName), line(1) { }

	virtual int sync();
	virtual std::streamsize xsputn(const char* s, std::streamsize n);

	char *fileName;
	int line;
};
	
extern OutputFormat outputFormat;
extern CodeStyleEnum codeStyle;

/* IO filenames and stream. */
extern bool printPrintables;
extern bool graphvizDone;

extern int gblErrorCount;
extern char machineMain[];

extern int numSplitPartitions;

std::ostream &error();
char *fileNameFromStem( char *stemFile, char *suffix );

#endif /* _RLGEN_JAVA_H */
